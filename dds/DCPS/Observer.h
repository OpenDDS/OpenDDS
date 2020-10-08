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

    Sample(DDS::InstanceHandle_t instance,
           DDS::InstanceStateKind instance_state,
           const DDS::Time_t& timestamp,
           const SequenceNumber& sequence_number,
           const void* data,
           const ValueWriterDispatcher& data_dispatcher);

    Sample(DDS::InstanceHandle_t instance,
           DDS::InstanceStateKind instance_state,
           const ReceivedDataElement& rde,
           const ValueWriterDispatcher& data_dispatcher);
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

  virtual ~Observer();
protected:
  Observer() {}
};

typedef RcHandle<Observer> Observer_rch;

OpenDDS_Dcps_Export void
vwrite(ValueWriter& vw, const Observer::Sample& sample);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_OBSERVER_H
