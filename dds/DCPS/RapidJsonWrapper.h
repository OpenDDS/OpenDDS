#ifndef OPENDDS_DCPS_RAPIDJSONWRAPPER_H
#define OPENDDS_DCPS_RAPIDJSONWRAPPER_H

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  if defined(__has_warning)
#    if __has_warning("-Wclass-memaccess")
#      pragma GCC diagnostic ignored "-Wclass-memaccess"
#    endif
#  elif __GNUC__ > 7
#    pragma GCC diagnostic ignored "-Wclass-memaccess"
#  endif
#endif
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#endif
