#ifndef OPENDDS_DCPS_V8TYPECONVERTER_H
#define OPENDDS_DCPS_V8TYPECONVERTER_H

#include <v8.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
  class V8TypeConverter {
  public:
    virtual v8::Local<v8::Object> toV8(const void* source) const = 0;
    virtual ~V8TypeConverter() {}
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
