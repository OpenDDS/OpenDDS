#ifndef OPENDDS_DCPS_RAPIDJSONTYPECONVERTER_H
#define OPENDDS_DCPS_RAPIDJSONTYPECONVERTER_H

#include "rapidjson/document.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class DataWriter;
}

namespace OpenDDS {
namespace DCPS {
  class RapidJsonTypeConverter {
  public:
    virtual ~RapidJsonTypeConverter() {}

    virtual void toRapidJson(const void* source, rapidjson::Value& dst, rapidjson::Value::AllocatorType& alloc) const = 0;
    virtual void* fromRapidJson(const rapidjson::Value& source) const = 0;
    virtual void deleteFromRapidJsonResult(void* obj) const = 0;

    virtual DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const = 0;
    virtual DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
    virtual DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
    virtual DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
