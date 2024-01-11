
#ifndef RX888_EXPORT_H
#define RX888_EXPORT_H

#if defined __GNUC__
#  if __GNUC__ >= 4
#    define __SDR_EXPORT   __attribute__((visibility("default")))
#    define __SDR_IMPORT   __attribute__((visibility("default")))
#  else
#    define __SDR_EXPORT
#    define __SDR_IMPORT
#  endif
#elif _MSC_VER
#  define __SDR_EXPORT     __declspec(dllexport)
#  define __SDR_IMPORT     __declspec(dllimport)
#else
#  define __SDR_EXPORT
#  define __SDR_IMPORT
#endif

#ifndef rx888_STATIC
#	ifdef rx888_EXPORTS
#	define RX888_API __SDR_EXPORT
#	else
#	define RX888_API __SDR_IMPORT
#	endif
#else
#define RX888_API
#endif
#endif /* RX888_EXPORT_H */
