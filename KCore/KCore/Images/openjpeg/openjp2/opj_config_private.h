/* create opj_config_private.h for CMake */
#define OPJ_HAVE_INTTYPES_H

#define OPJ_PACKAGE_VERSION "2.3.1"

/* Not used by openjp2*/
/*#cmakedefine HAVE_MEMORY_H @HAVE_MEMORY_H@*/
/*#cmakedefine HAVE_STDLIB_H @HAVE_STDLIB_H@*/
/*#cmakedefine HAVE_STRINGS_H @HAVE_STRINGS_H@*/
/*#cmakedefine HAVE_STRING_H @HAVE_STRING_H@*/
/*#cmakedefine HAVE_SYS_STAT_H @HAVE_SYS_STAT_H@*/
/*#cmakedefine HAVE_SYS_TYPES_H @HAVE_SYS_TYPES_H@ */
/*#cmakedefine HAVE_UNISTD_H @HAVE_UNISTD_H@*/

//#define _LARGEFILE_SOURCE
//#define _LARGE_FILES
//#define _FILE_OFFSET_BITS
#define OPJ_HAVE_FSEEKO

/* find whether or not have <malloc.h> */
#define OPJ_HAVE_MALLOC_H
/* check if function `aligned_alloc` exists */
//#define OPJ_HAVE_ALIGNED_ALLOC
/* check if function `_aligned_malloc` exists */
//#define OPJ_HAVE__ALIGNED_MALLOC
/* check if function `memalign` exists */
//#define OPJ_HAVE_MEMALIGN
/* check if function `posix_memalign` exists */
//#define OPJ_HAVE_POSIX_MEMALIGN

#if !defined(_POSIX_C_SOURCE)
#if defined(OPJ_HAVE_FSEEKO) || defined(OPJ_HAVE_POSIX_MEMALIGN)
/* Get declarations of fseeko, ftello, posix_memalign. */
#define _POSIX_C_SOURCE 200112L
#endif
#endif

/* Byte order.  */
/* All compilers that support Mac OS X define either __BIG_ENDIAN__ or
__LITTLE_ENDIAN__ to match the endianness of the architecture being
compiled for. This is not necessarily the same as the architecture of the
machine doing the building. In order to support Universal Binaries on
Mac OS X, we prefer those defines to decide the endianness.
On other platforms we use the result of the TRY_RUN. */
#if !defined(__APPLE__)
# define OPJ_BIG_ENDIAN
#elif defined(__BIG_ENDIAN__)
# define OPJ_BIG_ENDIAN
#endif
