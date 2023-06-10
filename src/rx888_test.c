/*
 * rx888_test.c - part of rx888 driver
 * rx888_test.c is part of rx888 driver
 *
 * Copyright (C) 2012-2014 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012-2014 by Kyle Keen <keenerd@gmail.com>
 * Copyright (C) 2014 by Michael Tatarinov <kukabu@gmail.com>
 * Copyright (C) 2022-2023 by Ruslan Migirov <trapi78@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#include <sys/time.h>

#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#include "getopt.h"
#endif

#include "librx888.h"

#define DEFAULT_SAMPLE_RATE		64000000
#define DEFAULT_BUF_LENGTH      (1024 * 16 * 8)
#define MINIMAL_BUF_LENGTH		1024
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

#define MHZ(x)	((x)*1000*1000)

#define PPM_DURATION			10
#define PPM_DUMP_TIME			5

struct time_generic
/* holds all the platform specific values */
{
#ifndef _WIN32
    time_t tv_sec;
    long tv_nsec;
#else
    long tv_sec;
    long tv_nsec;
    int init;
    LARGE_INTEGER frequency;
    LARGE_INTEGER ticks;
#endif
};

static enum {
    NO_BENCHMARK,
    TUNER_BENCHMARK,
    PPM_BENCHMARK
} test_mode = NO_BENCHMARK;

uint64_t total_bytes_received = 0;
struct timeval start_time;

static int do_exit = 0;
static rx888_dev_t *dev = NULL;

static uint32_t samp_rate = DEFAULT_SAMPLE_RATE;

static uint32_t total_samples = 0;
static uint32_t dropped_samples = 0;

static unsigned int ppm_duration = PPM_DURATION;

int verbose_set_sample_rate(rx888_dev_t *dev, uint32_t samp_rate)
{
    int r;
    r = rx888_set_sample_rate(dev, samp_rate);
    if (r < 0) {
        fprintf(stderr, "WARNING: Failed to set sample rate.\n");
    } else {
        fprintf(stderr, "Sampling at %u S/s.\n", samp_rate);
    }
    return r;
}


int verbose_device_search(char *s)
{
    int i, device_count, device, offset;
    char *s2;
    char vendor[256], product[256], serial[256];
    device_count = rx888_get_device_count();
    if (!device_count) {
        fprintf(stderr, "No supported devices found.\n");
        return -1;
    }
    fprintf(stderr, "Found %d device(s):\n", device_count);
    for (i = 0; i < device_count; i++) {
        rx888_get_device_usb_strings(i, vendor, product, serial);
        fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
    }
    fprintf(stderr, "\n");
    /* does string look like raw id number */
    device = (int)strtol(s, &s2, 0);
    if (s2[0] == '\0' && device >= 0 && device < device_count) {
        fprintf(stderr, "Using device %d: %s\n",
            device, rx888_get_device_name((uint32_t)device));
        return device;
    }
    /* does string exact match a serial */
    for (i = 0; i < device_count; i++) {
        rx888_get_device_usb_strings(i, vendor, product, serial);
        if (strcmp(s, serial) != 0) {
            continue;}
        device = i;
        fprintf(stderr, "Using device %d: %s\n",
            device, rx888_get_device_name((uint32_t)device));
        return device;
    }
    /* does string prefix match a serial */
    for (i = 0; i < device_count; i++) {
        rx888_get_device_usb_strings(i, vendor, product, serial);
        if (strncmp(s, serial, strlen(s)) != 0) {
            continue;}
        device = i;
        fprintf(stderr, "Using device %d: %s\n",
            device, rx888_get_device_name((uint32_t)device));
        return device;
    }
    /* does string suffix match a serial */
    for (i = 0; i < device_count; i++) {
        rx888_get_device_usb_strings(i, vendor, product, serial);
        offset = strlen(serial) - strlen(s);
        if (offset < 0) {
            continue;}
        if (strncmp(s, serial+offset, strlen(s)) != 0) {
            continue;}
        device = i;
        fprintf(stderr, "Using device %d: %s\n",
            device, rx888_get_device_name((uint32_t)device));
        return device;
    }
    fprintf(stderr, "No matching devices found.\n");
    return -1;
}

void usage(void)
{
    fprintf(stderr,
        "rx888_test, a benchmark tool for rx888 receiver\n\n"
        "Usage:\n"
        "\t[-s samplerate (default: 2048000 Hz)]\n"
        "\t[-d device_index (default: 0)]\n"
        "\t[-b output_block_size (default: 16 * 16384)]\n");
    exit(1);
}

#ifdef _WIN32
BOOL WINAPI
sighandler(int signum)
{
    if (CTRL_C_EVENT == signum) {
        fprintf(stderr, "Signal caught, exiting!\n");
        do_exit = 1;
        rx888_cancel_async(dev);
        return TRUE;
    }
    return FALSE;
}
#else
static void sighandler(int signum)
{
    (void)signum;
    fprintf(stderr, "Signal caught, exiting!\n");
    do_exit = 1;
    rx888_cancel_async(dev);
}
#endif

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    (void)buf;
    (void)ctx;
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    if (total_bytes_received == 0) {
        start_time = current_time;
    }

    total_bytes_received += len;
    double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                          ((current_time.tv_usec - start_time.tv_usec)/1000000.0);

    if (elapsed_time > 0) { // to prevent divide by zero
        double megabytes_per_second = (total_bytes_received / elapsed_time) / (1024 * 1024);
        printf("Data rate: %f MB/s\n", megabytes_per_second);
    }
}

int main(int argc, char **argv)
{
#ifndef _WIN32
    struct sigaction sigact;
#endif
    int r, opt;
    uint8_t *buffer;
    int dev_index = 0;
    int dev_given = 0;
    uint32_t out_block_size = DEFAULT_BUF_LENGTH;

    while ((opt = getopt(argc, argv, "d:s:b:tp::Sh")) != -1) {
        switch (opt) {
        case 'd':
            dev_index = verbose_device_search(optarg);
            dev_given = 1;
            break;
        case 's':
            samp_rate = (uint32_t)atof(optarg);
            break;
        case 'b':
            out_block_size = (uint32_t)atof(optarg);
            break;
        case 't':
            test_mode = TUNER_BENCHMARK;
            break;
        case 'p':
            test_mode = PPM_BENCHMARK;
            if (optarg)
                ppm_duration = atoi(optarg);
            break;
        case 'h':
        default:
            usage();
            break;
        }
    }

    if(out_block_size < MINIMAL_BUF_LENGTH ||
       out_block_size > MAXIMAL_BUF_LENGTH ){
        fprintf(stderr,
            "Output block size wrong value, falling back to default\n");
        fprintf(stderr,
            "Minimal length: %u\n", MINIMAL_BUF_LENGTH);
        fprintf(stderr,
            "Maximal length: %u\n", MAXIMAL_BUF_LENGTH);
        out_block_size = DEFAULT_BUF_LENGTH;
    }

    buffer = malloc(out_block_size * sizeof(uint8_t));

    if (!dev_given) {
        dev_index = verbose_device_search("0");
    }

    if (dev_index < 0) {
        exit(1);
    }

    r = rx888_open(&dev, (uint32_t)dev_index);
    if (r < 0) {
        fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
        exit(1);
    }
#ifndef _WIN32
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGPIPE, &sigact, NULL);
#else
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#endif
    
    /* Set the sample rate */
    verbose_set_sample_rate(dev, samp_rate);

    gettimeofday(&start_time, NULL);

    fprintf(stderr, "Reading samples in async mode...\n");
    r = rx888_read_async(dev, rtlsdr_callback, NULL,
                      0, out_block_size);
    
    if (do_exit) {
        fprintf(stderr, "\nUser cancel, exiting...\n");
        fprintf(stderr, "Samples per million lost (minimum): %i\n", (int)(1000000L * dropped_samples / total_samples));
    }
    else
        fprintf(stderr, "\nLibrary error %d, exiting...\n", r);

    rx888_close(dev);
    free (buffer);

    return r >= 0 ? r : -r;
}
