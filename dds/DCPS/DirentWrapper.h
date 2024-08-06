#ifndef OPENDDS_DCPS_DIRENTWRAPPER_H
#define OPENDDS_DCPS_DIRENTWRAPPER_H

#include "Definitions.h"

#if OPENDDS_GCC_HAS_DIAG_PUSHPOP
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
#if OPENDDS_GCC_HAS_DIAG_PUSHPOP
#  pragma GCC diagnostic pop
#endif

#endif
