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
//#include "LocalObject.h"
#include "SequenceNumber.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Observer
  : public virtual RcObject
{
public:
  typedef Observer* Ptr;
  typedef TAO_Objref_Var_T<Observer> Var;
  typedef TAO_Objref_Out_T<Observer> Out;
  typedef RcHandle<Observer> Rch;

  enum Event {
    e_ENABLED                = 0X0001 << 0,
    e_DELETED                = 0X0001 << 1,
    e_QOS_CHANGED            = 0X0001 << 2,
    e_ASSOCIATED             = 0X0001 << 3,
    e_DISASSOCIATED          = 0X0001 << 4,
    e_ASSOCIATED_QOS_CHANGED = 0X0001 << 5,
    e_SAMPLE_SENT            = 0X0001 << 6,
    e_SAMPLE_RECEIVED        = 0X0001 << 7,
    e_SAMPLE_READ            = 0X0001 << 8,
    e_SAMPLE_TAKEN           = 0X0001 << 9,
    e_NONE                   = 0X0000
  };

  struct Sample
  {
    GUID_t sender_;
    GUID_t receiver_;
    std::string topic_;
    SequenceNumber seq_n_;
    DDS::Time_t timestamp_;
    void* data_;
  };

  // 1) Reader/Writer enabled, QoS changed, deleted
  //    - Unique Id for the writer/reader, includes domain, topic, data type, QoS
  virtual void on_enabled(DDS::DataWriter_ptr) {}
  virtual void on_enabled(DDS::DataReader_ptr) {}

  virtual void on_deleted(DDS::DataWriter_ptr) {}
  virtual void on_deleted(DDS::DataReader_ptr) {}

  virtual void on_qos_changed(DDS::DataWriter_ptr) {}
  virtual void on_qos_changed(DDS::DataReader_ptr) {}

  // 2) Peer associated, QoS changed, association removed
  //    - Includes peer's unique Id, QoS
  virtual void on_associated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
  virtual void on_associated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}

  virtual void on_disassociated(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
  virtual void on_disassociated(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}

  virtual void on_associated_qos_changed(DDS::DataWriter_ptr, const GUID_t& /* readerId */) {}
  virtual void on_associated_qos_changed(DDS::DataReader_ptr, const GUID_t& /* writerId */) {}

  // 3) DataWriter sample sent
  //    - Writer Id, seq#, timestamp, instance handle, instance state, data payload
  //      - Data payloads will be in binary encoded (CDR) format
  //      - A utility function will be provided to convert CDR to JSON string
  virtual void on_sample_sent(const DDS::DataWriter_ptr, const Sample&) {}

  // 4) Sample received by DataReader
  //    - ReaderId, Same data fields as "sample sent" above
  virtual void on_sample_received(const DDS::DataReader_ptr, const Sample&) {}

  // 5) Sample received by application (read/take from DataReader)
  //    - ReaderId, Same data fields as "sample sent" above, read/take context
  virtual void on_sample_read(const DDS::DataReader_ptr, const Sample&) {}
  virtual void on_sample_taken(const DDS::DataReader_ptr, const Sample&) {}

  virtual ~Observer() {}
protected:
  Observer() {}
};

// comply with specification
typedef Observer::Ptr Observer_ptr;
typedef Observer::Var Observer_var;
typedef Observer::Out Observer_out;
typedef Observer::Rch Observer_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_OBSERVER_H
