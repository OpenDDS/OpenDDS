/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_DISPATCHER_H
#define OPENDDS_DCPS_VALUE_DISPATCHER_H

#include "TypeSupportImpl.h"
#include "ValueReader.h"
#include "ValueWriter.h"
#include "Sample.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Dcps_Export ValueDispatcher {
  virtual ~ValueDispatcher() {}

  // The void* is assumed to point to a value of type T (see template below).
  virtual void* new_value() const = 0;
  virtual void delete_value(void* data) const = 0;

  virtual bool read(ValueReader& value_reader, void* data, Sample::Extent ext = Sample::Full) const = 0;
  virtual bool write(ValueWriter& value_writer, const void* data, Sample::Extent ext = Sample::Full) const = 0;

  virtual DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const = 0;
  virtual DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
  virtual DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
  virtual DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const = 0;
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

  typedef typename OpenDDS::DCPS::DDSTraits<T> TraitsType;

  virtual bool read(ValueReader& value_reader, void* data, Sample::Extent ext = Sample::Full) const
  {
    switch (ext) {
    case Sample::Full:
      return vread(value_reader, *static_cast<T*>(data));
    case Sample::KeyOnly:
      return vread(value_reader, KeyOnly<T>(*static_cast<T*>(data)));
    default:
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ValueDispatcher_T<%C>::read:"
                   " Called with Sample::Extent NestedKeyOnly\n", TraitsType::type_name()));
      }
      return false;
    }
  }

  virtual bool write(ValueWriter& value_writer, const void* data, Sample::Extent ext = Sample::Full) const
  {
    switch (ext) {
    case Sample::Full:
      return vwrite(value_writer, *static_cast<const T*>(data));
    case Sample::KeyOnly:
      return vwrite(value_writer, KeyOnly<const T>(*static_cast<const T*>(data)));
    default:
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ValueDispatcher_T<%C>::write:"
                   " Called with Sample::Extent NestedKeyOnly\n", TraitsType::type_name()));
      }
      return false;
    }
  }

  typedef typename TraitsType::DataWriterType DataWriterType;

  virtual DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->register_instance(*static_cast<const T*>(data)) : DDS::HANDLE_NIL;
  }

  virtual DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->write(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }

  virtual DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->unregister_instance(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }

  virtual DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const
  {
    DataWriterType* dw_t = dynamic_cast<DataWriterType*>(dw);
    return dw_t ? dw_t->dispose(*static_cast<const T*>(data), inst) : DDS::RETCODE_BAD_PARAMETER;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_DISPATCHER_H */
