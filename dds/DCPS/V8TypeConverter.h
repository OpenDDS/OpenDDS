#ifndef OPENDDS_DCPS_V8TYPECONVERTER_H
#define OPENDDS_DCPS_V8TYPECONVERTER_H

#include <v8.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class DataWriter;
}

namespace OpenDDS {
namespace DCPS {
  class V8TypeConverter {
  public:
    virtual ~V8TypeConverter() {}

    virtual v8::Local<v8::Object> toV8(const void* source) const = 0;
    virtual void* fromV8(const v8::Local<v8::Object>& source) const = 0;
    virtual void deleteFromV8Result(void* obj) const = 0;

    virtual DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const = 0;
    virtual DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
    virtual DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
    virtual DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
