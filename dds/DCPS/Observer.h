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

#include <dds/DdsDcpsCoreC.h>
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
  typedef Observer* Ptr;
  typedef TAO_Objref_Var_T<Observer> Var;
  typedef TAO_Objref_Out_T<Observer> Out;
  typedef RcHandle<Observer> Rch;

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
    DDS::InstanceHandle_t instance_;
    CORBA::ULong instance_state_;
    DDS::Time_t timestamp_;
    SequenceNumber seq_n_;
    const void* data_;
    static CORBA::ULong to_instance_state(char message_id);
    Sample(DDS::InstanceHandle_t i, const DataSampleElement& e, const DDS::Time_t& t);
    explicit Sample(const ReceivedDataSample& s, DDS::InstanceHandle_t i = 0);
    explicit Sample(const ReceivedDataElement& s, DDS::InstanceHandle_t i = 0, CORBA::ULong instance_state = 0);
    Sample(DDS::InstanceHandle_t i, CORBA::ULong instance_state,
      const DDS::Time_t& t, const SequenceNumber& sn, const void* data);
  };

  // Group 1: Reader/Writer enabled, deleted, QoS changed
  virtual void on_enabled(DDS::DataWriter_ptr);
  virtual void on_enabled(DDS::DataReader_ptr);
  virtual void on_deleted(DDS::DataWriter_ptr);
  virtual void on_deleted(DDS::DataReader_ptr);
  virtual void on_qos_changed(DDS::DataWriter_ptr);
  virtual void on_qos_changed(DDS::DataReader_ptr);

  // Group 2: Peer associated, disassociated
  virtual void on_associated(DDS::DataWriter_ptr, const GUID_t& /* readerId */);
  virtual void on_associated(DDS::DataReader_ptr, const GUID_t& /* writerId */);
  virtual void on_disassociated(DDS::DataWriter_ptr, const GUID_t& /* readerId */);
  virtual void on_disassociated(DDS::DataReader_ptr, const GUID_t& /* writerId */);

  // Group 3: Sample sent, received, read, taken
  virtual void on_sample_sent(const DDS::DataWriter_ptr, const Sample&);
  virtual void on_sample_received(const DDS::DataReader_ptr, const Sample&);
  virtual void on_sample_read(const DDS::DataReader_ptr, const Sample&);
  virtual void on_sample_taken(const DDS::DataReader_ptr, const Sample&);

  virtual ~Observer();
protected:
  Observer() {}
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_OBSERVER_H
