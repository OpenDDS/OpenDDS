/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpDataLink.h"
#include "TcpReceiveStrategy.h"
#include "TcpInst.h"
#include "TcpSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportControlElement.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "ace/Log_Msg.h"

#if !defined (__ACE_INLINE__)
#include "TcpDataLink.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpDataLink::TcpDataLink(
  const ACE_INET_Addr& remote_address,
  OpenDDS::DCPS::TcpTransport&  transport_impl,
  Priority priority,
  bool        is_loopback,
  bool        is_active)
  : DataLink(transport_impl, priority, is_loopback, is_active),
    remote_address_(remote_address),
    graceful_disconnect_sent_(false),
    release_is_pending_(false)
{
  DBG_ENTRY_LVL("TcpDataLink","TcpDataLink",6);
}

OpenDDS::DCPS::TcpDataLink::~TcpDataLink()
{
  DBG_ENTRY_LVL("TcpDataLink","~TcpDataLink",6);
}

/// Called when the DataLink has been "stopped" for some reason.  It could
/// be called from the DataLink::transport_shutdown() method (when the
/// TransportImpl is handling a shutdown() call).  Or, it could be called
/// from the DataLink::release_reservations() method, when it discovers that
/// it has just released the last remaining reservations from the DataLink,
/// and the DataLink is in the process of "releasing" itself.
void
OpenDDS::DCPS::TcpDataLink::stop_i()
{
  DBG_ENTRY_LVL("TcpDataLink","stop_i",6);

  TcpConnection_rch connection(this->connection_.lock());
  if (connection) {
    // Tell the connection object to disconnect.
    connection->disconnect();
  }
}

void
OpenDDS::DCPS::TcpDataLink::pre_stop_i()
{
  DBG_ENTRY_LVL("TcpDataLink","pre_stop_i",6);

  DataLink::pre_stop_i();

  TcpReceiveStrategy_rch rs = this->receive_strategy();

  TcpConnection_rch connection(this->connection_.lock());

  if (rs) {
    // If we received the GRACEFUL_DISCONNECT message from peer before we
    // initiate the disconnecting of the datalink, then we will not send
    // the GRACEFUL_DISCONNECT message to the peer.
    bool disconnected = rs->gracefully_disconnected();

    if (connection && !this->graceful_disconnect_sent_
        && !disconnected && !this->impl().is_shut_down()) {
      this->send_graceful_disconnect_message();
      this->graceful_disconnect_sent_ = true;
    }
  }

  if (connection) {
    connection->shutdown();
  }
}

/// The TcpTransport calls this method when it has an established
/// connection object for us.  This call puts this TcpDataLink into
/// the "connected" state.
int
OpenDDS::DCPS::TcpDataLink::connect(
  const TcpConnection_rch& connection,
  const RcHandle<TcpSendStrategy>& send_strategy,
  const RcHandle<TcpReceiveStrategy>& receive_strategy)
{
  DBG_ENTRY_LVL("TcpDataLink","connect",6);

  this->connection_ = connection;

  if (connection->peer().enable(ACE_NONBLOCK) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpDataLink::connect failed to set "
                      "ACE_NONBLOCK %p\n", ACE_TEXT("enable")), -1);
  }

  // Let connection know the datalink for callbacks upon reconnect failure.
  connection->set_datalink(rchandle_from(this));

  // And lastly, inform our base class (DataLink) that we are now "connected",
  // and it should start the strategy objects.
  if (this->start(send_strategy, receive_strategy, false) != 0) {
    // Our base (DataLink) class failed to start the strategy objects.
    // We need to "undo" some things here before we return -1 to indicate
    // that an error has taken place.

    // Drop our reference to the connection object.
    this->connection_.reset();

    return -1;
  }

  do_association_actions();
  return 0;
}

//Allows the passive side to detect that the active side is connecting again
//prior to discovery identifying the released datalink from the active side.
//The passive side still believes it has a connection to the remote, however,
//the connect has created a new link/connection, thus the passive side can try
//to reuse the existing structures but reset it to associate the datalink with
//this new connection.
int
OpenDDS::DCPS::TcpDataLink::reuse_existing_connection(const TcpConnection_rch& connection)
{
  DBG_ENTRY_LVL("TcpDataLink","reuse_existing_connection",6);

  if (this->is_active_) {
    return -1;
  }
  //Need to check if connection is nil.  If connection is not nil, then connection
  //has previously gone through connection phase so this is a reuse of the connection
  //proceed to determine if we can reuse/reset existing mechanisms or need to start from
  //scratch.

  TcpConnection_rch old_connection(this->connection_.lock());

  if (old_connection) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpDataLink::reuse_existing_connection - "
                           "trying to reuse existing connection\n"), 0);
    old_connection->transfer(connection.in());

    //Connection already exists.
    TransportStrategy_rch brs;
    TransportSendStrategy_rch bss;

    if (this->receive_strategy_.is_nil() && this->send_strategy_.is_nil()) {
      return -1;
    } else {
      brs = this->receive_strategy_;
      bss = this->send_strategy_;

      this->connection_ = connection;

      TcpReceiveStrategy* rs = static_cast<TcpReceiveStrategy*>(brs.in());

      TcpSendStrategy* ss = static_cast<TcpSendStrategy*>(bss.in());

      // Associate the new connection object with the receiving strategy and disassociate
      // the old connection object with the receiving strategy.
      int rs_result = rs->reset(0, connection.in());

      // Associate the new connection object with the sending strategy and disassociate
      // the old connection object with the sending strategy.
      int ss_result = ss->reset(true);

      if (rs_result == 0 && ss_result == 0) {
        do_association_actions();
        return 0;
      }
    }
  }
  return -1;
}

/// Associate the new connection object with this datalink object.
/// The states of the "old" connection object are copied to the new
/// connection object and the "old" connection object is replaced by
/// the new connection object.
int
OpenDDS::DCPS::TcpDataLink::reconnect(const TcpConnection_rch& connection)
{
  DBG_ENTRY_LVL("TcpDataLink","reconnect",6);

  TcpConnection_rch existing_connection(this->connection_.lock());
  // Sanity check - the connection should exist already since we are reconnecting.
  if (!existing_connection) {
    VDBG_LVL((LM_ERROR,
              "(%P|%t) ERROR: TcpDataLink::reconnect old connection is nil.\n")
             , 1);
    return -1;
  }

  existing_connection->transfer(connection.in());

  bool released = false;
  TransportStrategy_rch brs;
  TransportSendStrategy_rch bss;

  {
    GuardType guard2(this->strategy_lock_);

    if (this->receive_strategy_.is_nil() && this->send_strategy_.is_nil()) {
      released = true;

    } else {
      brs = this->receive_strategy_;
      bss = this->send_strategy_;
    }
  }

  if (released) {
    int result = static_cast<TcpTransport&>(impl()).connect_tcp_datalink(*this, connection);
    if (result == 0) {
      do_association_actions();
    }
    return result;
  }

  this->connection_ = connection;

  TcpReceiveStrategy* rs = static_cast<TcpReceiveStrategy*>(brs.in());

  TcpSendStrategy* ss = static_cast<TcpSendStrategy*>(bss.in());

  // Associate the new connection object with the receiveing strategy and disassociate
  // the old connection object with the receiveing strategy.
  int rs_result = rs->reset(existing_connection.in(), connection.in());

  // Associate the new connection object with the sending strategy and disassociate
  // the old connection object with the sending strategy.
  int ss_result = ss->reset();

  if (rs_result == 0 && ss_result == 0) {
    do_association_actions();
    return 0;
  }

  return -1;
}

void
OpenDDS::DCPS::TcpDataLink::send_graceful_disconnect_message()
{
  DBG_ENTRY_LVL("TcpDataLink","send_graceful_disconnect_message",6);

  // Will clear all queued messages but still let the disconnect message
  // sent.
  this->send_strategy_->terminate_send(true);

  DataSampleHeader header_data;
  // The message_id_ is the most important value for the DataSampleHeader.
  header_data.message_id_ = GRACEFUL_DISCONNECT;

  // Other data in the DataSampleHeader are not necessary set. The bogus values
  // can be used.

  //header_data.byte_order_
  //  = this->transport_->config()->swap_bytes() ? !TAO_ENCAP_BYTE_ORDER : TAO_ENCAP_BYTE_ORDER;
  //header_data.message_length_ = 0;
  //header_data.sequence_ = 0;
  //DDS::Time_t source_timestamp
  //  = SystemTimePoint::now().to_dds_time();
  //header_data.source_timestamp_sec_ = source_timestamp.sec;
  //header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;
  //header_data.coherency_group_ = 0;
  //header_data.publication_id_ = 0;

  // TODO:
  // It seems a bug in the transport implementation that the receiving side can
  // not receive the message when the message has no sample data and is sent
  // in a single packet.

  // To work arround this problem, I have to add bogus data to chain with the
  // DataSampleHeader to make the receiving work.
  size_t max_marshaled_size = header_data.max_marshaled_size();

  Message_Block_Ptr data(
    new ACE_Message_Block(20,
                          ACE_Message_Block::MB_DATA,
                          0, //cont
                          0, //data
                          0, //allocator_strategy
                          0, //locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          0,
                          0));
  data->wr_ptr(20);

  header_data.message_length_ = static_cast<ACE_UINT32>(data->length());

  Message_Block_Ptr message(
    new ACE_Message_Block(max_marshaled_size,
                          ACE_Message_Block::MB_DATA,
                          data.release(), //cont
                          0, //data
                          0, //allocator_strategy
                          0, //locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          0,
                          0));

  *message << header_data;

  TransportControlElement* send_element = new TransportControlElement(move(message));

  // I don't want to rebuild a connection in order to send
  // a graceful disconnect message.
  this->send_i(send_element, false);
}

void
OpenDDS::DCPS::TcpDataLink::set_release_pending(bool flag)
{
  this->release_is_pending_ = flag;
}

bool
OpenDDS::DCPS::TcpDataLink::is_release_pending() const
{
  return this->release_is_pending_.value();
}

bool
OpenDDS::DCPS::TcpDataLink::handle_send_request_ack(TransportQueueElement* element)
{
  if (Transport_debug_level >= 1) {
    const GuidConverter converter(element->publication_id());
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TcpDataLink::handle_send_request_ack(%@) sequence number %q, publication_id=%C\n"),
      element, element->sequence().getValue(), OPENDDS_STRING(converter).c_str()));
  }

  ACE_Guard<ACE_SYNCH_MUTEX> guard(pending_request_acks_lock_);
  pending_request_acks_.push_back(element);
  return false;
}


void
OpenDDS::DCPS::TcpDataLink::ack_received(const ReceivedDataSample& sample)
{
  SequenceNumber sequence = sample.header_.sequence_;

  if (sample.header_.sequence_ == -1) {
    return;
  }

  if (Transport_debug_level >= 1) {
    const GuidConverter converter(sample.header_.publication_id_);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TcpDataLink::ack_received() received sequence number %q, publiction_id=%C\n"),
      sequence.getValue(), OPENDDS_STRING(converter).c_str()));
  }

  TransportQueueElement* elem=0;
  {
    // find the pending request with the same sequence number.
    ACE_Guard<ACE_SYNCH_MUTEX> guard(pending_request_acks_lock_);
    PendingRequestAcks::iterator it;
    for (it = pending_request_acks_.begin(); it != pending_request_acks_.end(); ++it){
      if ((*it)->sequence() == sequence && (*it)->publication_id() == sample.header_.publication_id_) {
        elem = *it;
        pending_request_acks_.erase(it);
        break;
      }
    }
  }

  if (elem) {
    if (Transport_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TcpDataLink::ack_received() found matching element %@\n"),
        elem));
    }
    this->send_strategy_->deliver_ack_request(elem);
  }
  else {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TcpDataLink::ack_received() received unknown sequence number %q\n"),
      sequence.getValue()));
  }
}

void
OpenDDS::DCPS::TcpDataLink::request_ack_received(const ReceivedDataSample& sample)
{
  if (sample.header_.sequence_ == -1 && sample.header_.message_length_ == sizeof(RepoId)) {
    RepoId local;
    DCPS::Serializer ser(&(*sample.sample_));
    if (ser >> local) {
      invoke_on_start_callbacks(local, sample.header_.publication_id_, true);
    }
    return;
  }

  DataSampleHeader header_data;
  // The message_id_ is the most important value for the DataSampleHeader.
  header_data.message_id_ = SAMPLE_ACK;

  // Other data in the DataSampleHeader are not necessary set. The bogus values
  // can be used.

  header_data.byte_order_  = ACE_CDR_BYTE_ORDER;
  header_data.message_length_ = 0;
  header_data.sequence_ = sample.header_.sequence_;
  header_data.publication_id_ = sample.header_.publication_id_;
  header_data.publisher_id_ = sample.header_.publisher_id_;

  size_t max_marshaled_size = header_data.max_marshaled_size();

  Message_Block_Ptr message(
    new ACE_Message_Block(max_marshaled_size,
                          ACE_Message_Block::MB_DATA,
                          0, //cont
                          0, //data
                          0, //allocator_strategy
                          0, //locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          0,
                          0));

  *message << header_data;

  TransportControlElement* send_element =  new TransportControlElement(move(message));


  // I don't want to rebuild a connection in order to send
  // a sample ack message
  this->send_i(send_element, false);
}

void
OpenDDS::DCPS::TcpDataLink::do_association_actions()
{
  typedef std::vector<std::pair<RepoId, RepoId> > PairVec;
  PairVec to_send;
  PairVec to_call;

  {
    GuardType guard(strategy_lock_);

    for (OnStartCallbackMap::const_iterator it = on_start_callbacks_.begin(); it != on_start_callbacks_.end(); ++it) {
      for (RepoToClientMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
        GuidConverter conv(it2->first);
        to_send.push_back(std::make_pair(it2->first, it->first));
        if (!conv.isWriter()) {
          to_call.push_back(std::make_pair(it2->first, it->first));
        }
      }
    }
  }

  send_strategy_->link_released(false);

  for (PairVec::const_iterator it = to_call.begin(); it != to_call.end(); ++it) {
    invoke_on_start_callbacks(it->first, it->second, true);
  }

  for (PairVec::const_iterator it = to_send.begin(); it != to_send.end(); ++it) {
    send_association_msg(it->first, it->second);
  }
}

void
OpenDDS::DCPS::TcpDataLink::send_association_msg(const RepoId& local, const RepoId& remote)
{
  DataSampleHeader header_data;
  header_data.message_id_ = REQUEST_ACK;
  header_data.byte_order_  = ACE_CDR_BYTE_ORDER;
  header_data.message_length_ = sizeof(remote);
  header_data.sequence_ = -1;
  header_data.publication_id_ = local;
  header_data.publisher_id_ = remote;

  size_t max_marshaled_size = header_data.max_marshaled_size();

  Message_Block_Ptr message(
    new ACE_Message_Block(max_marshaled_size,
                          ACE_Message_Block::MB_DATA,
                          0, //cont
                          0, //data
                          0, //allocator_strategy
                          0, //locking_strategy
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          0,
                          0));

  *message << header_data;
  DCPS::Serializer ser(message.get());
  ser << remote;

  TransportControlElement* send_element = new TransportControlElement(move(message));

  this->send_i(send_element, false);
}

void
OpenDDS::DCPS::TcpDataLink::drop_pending_request_acks()
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(pending_request_acks_lock_);
  PendingRequestAcks::iterator it;
  for (it = pending_request_acks_.begin(); it != pending_request_acks_.end(); ++it){
    (*it)->data_dropped(true);
  }
  pending_request_acks_.clear();
}

OpenDDS::DCPS::TcpSendStrategy_rch
OpenDDS::DCPS::TcpDataLink::send_strategy()
{
  return static_rchandle_cast<OpenDDS::DCPS::TcpSendStrategy>(send_strategy_);
}

OpenDDS::DCPS::TcpReceiveStrategy_rch
OpenDDS::DCPS::TcpDataLink::receive_strategy()
{
  return static_rchandle_cast<OpenDDS::DCPS::TcpReceiveStrategy>(receive_strategy_);
}
int
OpenDDS::DCPS::TcpDataLink::make_reservation(const RepoId& remote_subscription_id,
                                             const RepoId& local_publication_id,
                                             const TransportSendListener_wrch& send_listener,
                                             bool reliable)
{
  const int result = DataLink::make_reservation(remote_subscription_id, local_publication_id, send_listener, reliable);
  send_association_msg(local_publication_id, remote_subscription_id);
  return result;
}

int
OpenDDS::DCPS::TcpDataLink::make_reservation(const RepoId& remote_publication_id,
                                             const RepoId& local_subscription_id,
                                             const TransportReceiveListener_wrch& receive_listener,
                                             bool reliable)
{
  const int result = DataLink::make_reservation(remote_publication_id, local_subscription_id, receive_listener, reliable);
  send_association_msg(local_subscription_id, remote_publication_id);
  return result;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
