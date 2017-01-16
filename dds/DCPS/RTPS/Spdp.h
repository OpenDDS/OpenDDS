/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SPDP_H
#define OPENDDS_RTPS_SPDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcEventHandler.h"

#include "RtpsCoreC.h"
#include "Sedp.h"
#include "rtps_export.h"

#include "ace/Atomic_Op.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Condition_Thread_Mutex.h"

#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/PoolAllocationBase.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class OpenDDS_Rtps_Export Spdp : public OpenDDS::DCPS::LocalParticipant<Sedp> {
public:
  Spdp(DDS::DomainId_t domain, DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco);
  ~Spdp();

  // Participant
  void init_bit(const DDS::Subscriber_var& bit_subscriber);
  void fini_bit();

  bool get_default_locators(const DCPS::RepoId& part_id,
                            OpenDDS::DCPS::LocatorSeq& target,
                            bool& inlineQos);

  // Managing reader/writer associations
  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  // Is Spdp shutting down?
  bool shutting_down() { return shutdown_flag_.value(); }

  bool associated() const;
  bool has_discovered_participant(const DCPS::RepoId& guid);

  WaitForAcks& wait_for_acks();

protected:
  Sedp& endpoint_manager() { return sedp_; }

private:
  ACE_Reactor* reactor() const;

  RtpsDiscovery* disco_;

  // Participant:
  const DDS::DomainId_t domain_;
  DCPS::RepoId guid_;
  OpenDDS::DCPS::LocatorSeq sedp_unicast_, sedp_multicast_;

  void data_received(const DataSubmessage& data, const ParameterList& plist);

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  struct SpdpTransport : public OpenDDS::DCPS::RcEventHandler, public OpenDDS::DCPS::PoolAllocationBase {
    explicit SpdpTransport(Spdp* outer);
    ~SpdpTransport();

    virtual int handle_timeout(const ACE_Time_Value&, const void*);
    virtual int handle_input(ACE_HANDLE h);
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

    void open();
    void write();
    void write_i();
    void close();
    void dispose_unregister();
    bool open_unicast_socket(u_short port_common, u_short participant_id);
    void acknowledge();

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    ACE_Time_Value lease_duration_;
    ACE_SOCK_Dgram unicast_socket_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
    OPENDDS_SET(ACE_INET_Addr) send_addrs_;
    ACE_Message_Block buff_, wbuff_;

  } *tport_;

  ACE_Event_Handler_var eh_; // manages our refcount on tport_
  bool eh_shutdown_;
  ACE_Condition_Thread_Mutex shutdown_cond_;
  ACE_Atomic_Op<ACE_Thread_Mutex, long> shutdown_flag_; // Spdp shutting down

  void remove_expired_participants();
  void get_discovered_participant_ids(DCPS::RepoIdSet& results) const;

  Sedp sedp_;
  // wait for acknowledgments from SpdpTransport and Sedp::Task
  // when BIT is being removed (fini_bit)
  WaitForAcks wait_for_acks_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SPDP_H
