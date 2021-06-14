#ifndef OPENDDS_DCPS_DIRENTWRAPPER_H
#define OPENDDS_DCPS_DIRENTWRAPPER_H

#if defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5) || defined __clang__)
#  define OPENDDS_GCC_HAS_DIAG_PUSHPOP
#  pragma GCC diagnostic push
#  if defined(__has_warning)
#    if __has_warning("-Wdeprecated-declarations")
#      pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#    endif
#  elif __GNUC__ >= 5
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#  endif
#endif
#include <ace/Dirent.h>
#ifdef OPENDDS_GCC_HAS_DIAG_PUSHPOP
#  pragma GCC diagnostic pop
#endif

#endif
