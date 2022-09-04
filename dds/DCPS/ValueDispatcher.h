/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_DISPATCHER_H
#define OPENDDS_DCPS_VALUE_DISPATCHER_H

#include "TypeSupportImpl.h"
#include "ValueReader.h"
#include "ValueWriter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Dcps_Export ValueDispatcher {
  virtual ~ValueDispatcher() {}

  virtual void* new_value() const = 0;
  virtual void delete_value(void* data) const = 0;

  virtual void read(ValueReader& value_reader, void* data) const = 0;
  virtual void write(ValueWriter& value_writer, const void* data) const = 0;

};

template <typename T>
struct ValueDispatcher_T : public virtual ValueDispatcher {
  virtual ~ValueDispatcher_T() {}

  virtual void* new_value() const
  {
    return new T();
  }

  virtual void delete_value(void* data) const
  {
    T* tbd = static_cast<T*>(data);
    delete tbd;
  }

  virtual void read(ValueReader& value_reader, void* data) const
  {
    vread(value_reader, *static_cast<T*>(data));
  }

  virtual void write(ValueWriter& value_writer, const void* data) const
  {
    vwrite(value_writer, *static_cast<const T*>(data));
  }

  typedef typename DDSTraits<T>::DataWriterType DataWriterType;

  DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->register_instance(*static_cast<const T*>(data)) : DDS::HANDLE_NIL;
  }

  DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->write(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->unregister_instance(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->dispose(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_DISPATCHER_H */
