#ifndef OPENDDS_DCPS_V8TYPECONVERTER_H
#define OPENDDS_DCPS_V8TYPECONVERTER_H

namespace v8 {
  class Value;
}

namespace OpenDDS {
namespace DCPS {
  class V8TypeConverter {
  public:
    virtual v8::Value* toV8(const void* source) const = 0;
  };
}
}

#endif
