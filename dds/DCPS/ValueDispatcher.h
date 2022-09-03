/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_DISPATCHER_H
#define OPENDDS_DCPS_VALUE_DISPATCHER_H

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
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_DISPATCHER_H */
