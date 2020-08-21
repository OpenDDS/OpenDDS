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

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct SampleMetaData
{
  //TODO: add fields
};

class Observer;
typedef Observer* Observer_ptr;
typedef TAO_Objref_Var_T<Observer> Observer_var;
typedef TAO_Objref_Out_T<Observer> Observer_out;

class Observer
  : public virtual RcObject
{
public:
  enum EventMask {
    Reader_Enabled                = 0x0001 << 0,
    Writer_Enabled                = 0x0001 << 1,
    Reader_Deleted                = 0x0001 << 2,
    Writer_Deleted                = 0x0001 << 3,
    Reader_QoS_Changed            = 0x0001 << 4,
    Writer_QoS_Changed            = 0x0001 << 5,
    Reader_Associated             = 0x0001 << 6,
    Writer_Associated             = 0x0001 << 7,
    Reader_Disassociated          = 0x0001 << 8,
    Writer_Disassociated          = 0x0001 << 9,
    Associated_Reader_QoS_Changed = 0x0001 << 10,
    Associated_Writer_QoS_Changed = 0x0001 << 11,
    Sample_Sent                   = 0x0001 << 12,
    Sample_Received               = 0x0001 << 13,
    Sample_Read                   = 0x0001 << 14,
    Sample_Taken                  = 0x0001 << 15,
    None                          = 0x0000
  };

  // 1) Reader/Writer enabled, QoS changed, deleted
  //    - Unique Id for the writer/reader, includes domain, topic, data type, QoS
  virtual void on_enabled(DDS::DataReader_ptr) {}
  virtual void on_enabled(DDS::DataWriter_ptr) {}

  virtual void on_deleted(DDS::DataReader_ptr) {}
  virtual void on_deleted(DDS::DataWriter_ptr) {}

  virtual void on_qos_changed(DDS::DataReader_ptr) {}
  virtual void on_qos_changed(DDS::DataWriter_ptr) {}

  // 2) Peer associated, QoS changed, association removed
  //    - Includes peer's unique Id, QoS
  virtual void on_associated(DDS::DataReader_ptr, const RepoId& /* writerId */) {}
  virtual void on_associated(DDS::DataWriter_ptr, const RepoId& /* readerId */) {}

  virtual void on_disassociated(DDS::DataReader_ptr, const RepoId& /* writerId */) {}
  virtual void on_disassociated(DDS::DataWriter_ptr, const RepoId& /* readerId */) {}

  virtual void on_associated_qos_changed(DDS::DataWriter_ptr) {}
  virtual void on_associated_qos_changed(DDS::DataReader_ptr) {}

  // 3) DataWriter sample sent
  //    - Writer Id, seq#, timestamp, instance handle, instance state, data payload
  //      - Data payloads will be in binary encoded (CDR) format
  //      - A utility function will be provided to convert CDR to JSON string
  virtual void on_sample_sent(
    const DDS::DataWriter_ptr, const SampleMetaData&, const void* /* sample */) {}

  // 4) Sample received by DataReader
  //    - ReaderId, Same data fields as "sample sent" above
  virtual void on_sample_received(
    const DDS::DataReader_ptr, const SampleMetaData&, const void* /* sample */) {}

  // 5) Sample received by application (read/take from DataReader)
  //    - ReaderId, Same data fields as "sample sent" above, read/take context
  virtual void on_sample_read(
    const DDS::DataReader_ptr, const SampleMetaData&, const void* /* sample */) {}

  virtual void on_sample_taken(
    const DDS::DataReader_ptr, const SampleMetaData&, const void* /* sample */) {}

  //combine on_sample_received/read/taken with EventMask
  //virtual void on_sample_received(
  //  const DDS::DataReader_ptr, const SampleMetaData&, const void* /* sample */, const EventMask) {}

  virtual ~Observer() {}
protected:
  Observer() {}
};

typedef RcHandle<Observer> Observer_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_OBSERVER_H
