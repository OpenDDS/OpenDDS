/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_OBSERVER_H
#define OPENDDS_DCPS_OBSERVER_H

#include "RcObject.h"
#include "Definitions.h"
#include "SequenceNumber.h"
#include "ValueWriter.h"
#include "ReceivedDataElementList.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;
class ReceivedDataSample;
class ReceivedDataElement;

class OpenDDS_Dcps_Export Observer
  : public virtual RcObject
{
public:
  typedef unsigned long Event;
  enum {
    e_ENABLED         = 0x0001 << 0,
    e_DELETED         = 0x0001 << 1,
    e_QOS_CHANGED     = 0x0001 << 2,
    e_ASSOCIATED      = 0x0001 << 3,
    e_DISASSOCIATED   = 0x0001 << 4,
    e_SAMPLE_SENT     = 0x0001 << 5,
    e_SAMPLE_RECEIVED = 0x0001 << 6,
    e_SAMPLE_READ     = 0x0001 << 7,
    e_SAMPLE_TAKEN    = 0x0001 << 8,
    e_NONE            = 0x0000,
    e_ALL             = 0xffff
  };

  struct OpenDDS_Dcps_Export Sample
  {
    DDS::InstanceHandle_t instance;
    DDS::InstanceStateKind instance_state;
    DDS::Time_t timestamp;
    SequenceNumber sequence_number;
    const void* data;
    const ValueWriterDispatcher& data_dispatcher;

    Sample(DDS::InstanceHandle_t a_instance,
           DDS::InstanceStateKind a_instance_state,
           const DDS::Time_t& a_timestamp,
           const SequenceNumber& a_sequence_number,
           const void* a_data,
           const ValueWriterDispatcher& a_data_dispatcher)
      : instance(a_instance)
      , instance_state(a_instance_state)
      , timestamp(a_timestamp)
      , sequence_number(a_sequence_number)
      , data(a_data)
      , data_dispatcher(a_data_dispatcher)
    {}

    Sample(DDS::InstanceHandle_t a_instance,
           DDS::InstanceStateKind a_instance_state,
           const ReceivedDataElement& a_rde,
           const ValueWriterDispatcher& a_data_dispatcher)
      : instance(a_instance)
      , instance_state(a_instance_state)
      , timestamp(a_rde.source_timestamp_)
      , sequence_number(a_rde.sequence_)
      , data(a_rde.registered_data_)
      , data_dispatcher(a_data_dispatcher)
    {}
  };

  // Group 1: Reader/Writer enabled, deleted, QoS changed
  virtual void on_enabled(DDS::DataWriter_ptr) {}
  virtual void on_enabled(DDS::DataReader_ptr) {}
  virtual void on_deleted(DDS::DataWriter_ptr) {}
  virtual void on_deleted(DDS::DataReader_ptr) {}
  virtual void on_qos_changed(DDS::DataWriter_ptr) {}
  virtual void on_qos_changed(DDS::DataReader_ptr) {}

  // Group 2: Peer associated, disassociated
  virtual void on_associated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
  virtual void on_associated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}
  virtual void on_disassociated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
  virtual void on_disassociated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}

  // Group 3: Sample sent, received, read, taken
  virtual void on_sample_sent(DDS::DataWriter_ptr, const Sample&) {}
  virtual void on_sample_received(DDS::DataReader_ptr, const Sample&) {}
  virtual void on_sample_read(DDS::DataReader_ptr, const Sample&) {}
  virtual void on_sample_taken(DDS::DataReader_ptr, const Sample&) {}

  virtual ~Observer() {}
protected:
  Observer() {}
};

typedef RcHandle<Observer> Observer_rch;

inline void
vwrite(ValueWriter& vw, const Observer::Sample& sample)
{
  vw.begin_struct();
  vw.begin_field("instance");
  vw.write_int32(sample.instance);
  vw.end_field();
  vw.begin_field("instance_state");
  vw.write_uint32(sample.instance_state);
  vw.end_field();
  vw.begin_field("timestamp");
  vwrite(vw, sample.timestamp);
  vw.end_field();
  vw.begin_field("sequence_number");
  vw.write_int64(sample.sequence_number.getValue());
  vw.end_field();
  vw.begin_field("data");
  sample.data_dispatcher.write(vw, sample.data);
  vw.end_field();
  vw.end_struct();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_OBSERVER_H
