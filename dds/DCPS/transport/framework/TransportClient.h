/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCLIENT_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCLIENT_H

#include "TransportConfig_rch.h"
#include "TransportImpl.h"
#include "DataLinkSet.h"

#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/AssociationData.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/DiscoveryListener.h>
#include <dds/DCPS/RcEventHandler.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/GuidUtils.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <ace/Time_Value.h>
#include <ace/Event_Handler.h>
#include <ace/Reverse_Lock_T.h>

// Forward definition of a test-friendly class in the global name space
class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SendStateDataSampleList;

/**
 * @brief Mix-in class for DDS entities which directly use the transport layer.
 *
 * DataReaderImpl and DataWriterImpl are TransportClients.  The TransportClient
 * class manages the TransportImpl objects that represent the available
 * communication mechanisms and the DataLink objects that represent the
 * currently active communication channels to peers.
 */
class OpenDDS_Dcps_Export TransportClient
  : public virtual RcObject
{
public:
  // Used by TransportImpl to complete associate() processing:
  void use_datalink(const GUID_t& remote_id, const DataLink_rch& link);

  // values for flags parameter of transport_assoc_done():
  enum { ASSOC_OK = 1, ASSOC_ACTIVE = 2 };
  TransportClient();
  virtual ~TransportClient();


  // Local setup:

  void enable_transport(bool reliable, bool durable, DomainParticipantImpl* dpi);
  void enable_transport_using_config(bool reliable, bool durable,
                                     const TransportConfig_rch& tc,
                                     DomainParticipantImpl* dpi);

  bool swap_bytes() const { return swap_bytes_; }
  bool cdr_encapsulation() const { return cdr_encapsulation_; }
  const TransportLocatorSeq& connection_info() const { return conn_info_; }
  void populate_connection_info(DomainParticipantImpl* dpi);
  bool is_reliable() const { return reliable_; }

  // Managing associations to remote peers:

  bool associate(const AssociationData& peer, bool active);
  void disassociate(const GUID_t& peerId);
  void stop_associating();
  void stop_associating(const GUID_t* repos, CORBA::ULong length);
  void send_final_acks();
  void transport_stop();

  // Discovery:
  void register_for_reader(const GUID_t& participant,
                           const GUID_t& writerid,
                           const GUID_t& readerid,
                           const TransportLocatorSeq& locators,
                           OpenDDS::DCPS::DiscoveryListener* listener);

  void unregister_for_reader(const GUID_t& participant,
                             const GUID_t& writerid,
                             const GUID_t& readerid);

  void register_for_writer(const GUID_t& participant,
                           const GUID_t& readerid,
                           const GUID_t& writerid,
                           const TransportLocatorSeq& locators,
                           DiscoveryListener* listener);

  void unregister_for_writer(const GUID_t& participant,
                             const GUID_t& readerid,
                             const GUID_t& writerid);

  void update_locators(const GUID_t& remote,
                       const TransportLocatorSeq& locators);

  WeakRcHandle<ICE::Endpoint> get_ice_endpoint();

  // Data transfer:

  bool send_response(const GUID_t& peer,
                     const DataSampleHeader& header,
                     Message_Block_Ptr payload); // [DR]

  void send(SendStateDataSampleList send_list, ACE_UINT64 transaction_id = 0);

  SendControlStatus send_w_control(SendStateDataSampleList send_list,
                                   const DataSampleHeader& header,
                                   Message_Block_Ptr msg,
                                   const GUID_t& destination);

  SendControlStatus send_control(const DataSampleHeader& header,
                                 Message_Block_Ptr msg);

  SendControlStatus send_control_to(const DataSampleHeader& header,
                                    Message_Block_Ptr msg,
                                    const GUID_t& destination);

  bool remove_sample(const DataSampleElement* sample);
  bool remove_all_msgs();

  virtual void add_link(const DataLink_rch& link, const GUID_t& peer);
  virtual RcHandle<BitSubscriber> get_builtin_subscriber_proxy() const { return RcHandle<BitSubscriber>(); }

  void terminate_send_if_suspended();

  bool associated_with(const GUID_t& remote) const;
  bool pending_association_with(const GUID_t& remote) const;

  void set_guid(const GUID_t& guid)
  {
    OPENDDS_ASSERT(guid_ == GUID_UNKNOWN);
    OPENDDS_ASSERT(guid != GUID_UNKNOWN);
    guid_ = guid;
  }

  GUID_t get_guid() const
  {
    OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
    return guid_;
  }

  void data_acked(const GUID_t& remote);

  SequenceNumber cur_cumulative_ack(const GUID_t& reader_id) const;
  bool is_leading(const GUID_t& reader_id) const;

protected:
  void cdr_encapsulation(bool encap)
  {
    cdr_encapsulation_ = encap;
  }

private:

  // Implemented by derived classes (DataReaderImpl/DataWriterImpl)
  virtual bool check_transport_qos(const TransportInst& inst) = 0;
  virtual DDS::DomainId_t domain_id() const = 0;
  virtual Priority get_priority_value(const AssociationData& data) const = 0;
  virtual void transport_assoc_done(int /*flags*/, const GUID_t& /*remote*/) {}
  virtual SequenceNumber get_max_sn() const { return SequenceNumber::SEQUENCENUMBER_UNKNOWN(); };



#if OPENDDS_CONFIG_SECURITY
  virtual DDS::Security::ParticipantCryptoHandle get_crypto_handle() const
  {
    return DDS::HANDLE_NIL;
  }
#endif

  // helpers
  typedef ACE_Guard<ACE_Thread_Mutex> Guard;
  void use_datalink_i(const GUID_t& remote_id,
                      const DataLink_rch& link,
                      Guard& guard);
  TransportSendListener_rch get_send_listener();
  TransportReceiveListener_rch get_receive_listener();

  //helper for initiating connection, called by PendingAssoc objects
  //allows PendingAssoc to temporarily release lock_ to allow
  //TransportImpl to access Reactor if needed
  bool initiate_connect_i(TransportImpl::AcceptConnectResult& result,
                          TransportImpl_rch impl,
                          const TransportImpl::RemoteTransport& remote,
                          const TransportImpl::ConnectionAttribs& attribs_,
                          Guard& guard);

  void send_i(SendStateDataSampleList send_list, ACE_UINT64 transaction_id);

  // A class, normally provided by an unit test, who needs access to a client's
  // privates.
  friend class ::DDS_TEST;

  typedef OPENDDS_MAP_CMP(GUID_t, DataLink_rch, GUID_tKeyLessThan) DataLinkIndex;
  typedef OPENDDS_VECTOR(WeakRcHandle<TransportImpl>) ImplsType;

  typedef ACE_Reverse_Lock<ACE_Thread_Mutex> Reverse_Lock_t;
  struct PendingAssoc : RcObject {
    ACE_Thread_Mutex mutex_;
    bool active_;
    bool scheduled_;
    ImplsType impls_;
    CORBA::ULong blob_index_;
    AssociationData data_;
    TransportImpl::ConnectionAttribs attribs_;
    WeakRcHandle<TransportClient> client_;

    explicit PendingAssoc(RcHandle<TransportClient> tc_rch)
      : active_(false)
      , scheduled_(false)
      , blob_index_(0)
      , client_(tc_rch)
      , timeout_task_(make_rch<PendingAssocSporadicTask>(TheServiceParticipant->time_source(), TheServiceParticipant->reactor_task(), rchandle_from(this), &PendingAssoc::timeout))
    {}

    ~PendingAssoc()
    {
      timeout_task_->cancel();
    }

    void reset_client();
    bool safe_to_remove();
    bool initiate_connect(TransportClient* tc, Guard& guard);

    void schedule(const TimeDuration& delay)
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      timeout_task_->schedule(delay);
      scheduled_ = true;
    }

    void cancel()
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      timeout_task_->cancel();
      scheduled_ = false;
    }

  private:
    typedef PmfSporadicTask<PendingAssoc> PendingAssocSporadicTask;
    RcHandle<PendingAssocSporadicTask> timeout_task_;
    void timeout(const MonotonicTimePoint& now);
  };

  typedef RcHandle<PendingAssoc> PendingAssoc_rch;

  typedef OPENDDS_MAP_CMP(GUID_t, PendingAssoc_rch, GUID_tKeyLessThan) PendingMap;
  typedef OPENDDS_MULTIMAP_CMP(GUID_t, PendingAssoc_rch, GUID_tKeyLessThan) PrevPendingMap;

  void clean_prev_pending();

  // Associated Impls and DataLinks:

  TransportConfig_rch config_;
  ImplsType impls_;
  PendingMap pending_;
  PrevPendingMap prev_pending_;
  DataLinkSet links_;

  DataLinkIndex data_link_index_;

  // Used to allow sends to completed as a transaction and block
  // multi-threaded writers from proceeding to send data
  // on two thread simultaneously, which could cause out-of-order data.
  ACE_Thread_Mutex send_transaction_lock_;
  ACE_UINT64 expected_transaction_id_;
  ACE_UINT64 max_transaction_id_seen_;

  //max_transaction_tail_ will always be the tail of the
  //max transaction that has been observed or 0 if this is
  //the first transaction or a transaction after the expected
  //value was met and thus reset to 0 indicating the samples were
  //sent up to max_transaction_id_
  DataSampleElement* max_transaction_tail_;

  // Configuration details:

  bool swap_bytes_, cdr_encapsulation_, reliable_, durable_;

  TimeDuration passive_connect_duration_;

  TransportLocatorSeq conn_info_;

  /// Seems to protect accesses to impls_, pending_, links_, data_link_index_
  mutable ACE_Thread_Mutex lock_;

  Reverse_Lock_t reverse_lock_;

  GUID_t guid_;
};

typedef RcHandle<TransportClient> TransportClient_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
