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
#include "ace/Log_Msg.h"

#if !defined (__ACE_INLINE__)
#include "TcpDataLink.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpDataLink::TcpDataLink(
  const ACE_INET_Addr& remote_address,
  const OpenDDS::DCPS::TcpTransport_rch&  transport_impl,
  Priority priority,
  bool        is_loopback,
  bool        is_active)
  : DataLink(transport_impl, priority, is_loopback, is_active),
    remote_address_(remote_address),
    transport_(transport_impl),
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

  if (!this->connection_.is_nil()) {
    // Tell the connection object to disconnect.
    this->connection_->disconnect();

    // Drop our reference to the connection object.
    this->connection_.reset();
  }
}

void
OpenDDS::DCPS::TcpDataLink::pre_stop_i()
{
  DBG_ENTRY_LVL("TcpDataLink","pre_stop_i",6);

  DataLink::pre_stop_i();

  TcpReceiveStrategy * rs
    = dynamic_cast <TcpReceiveStrategy*>(this->receive_strategy_.in());

  if (rs != NULL) {
    // If we received the GRACEFUL_DISCONNECT message from peer before we
    // initiate the disconnecting of the datalink, then we will not send
    // the GRACEFUL_DISCONNECT message to the peer.
    bool disconnected = rs->gracefully_disconnected();

    if (!this->connection_.is_nil() && !this->graceful_disconnect_sent_
        && !disconnected) {
      this->send_graceful_disconnect_message();
      this->graceful_disconnect_sent_ = true;
    }
  }

  if (!this->connection_.is_nil()) {
    this->connection_->shutdown();
  }
}

/// The TcpTransport calls this method when it has an established
/// connection object for us.  This call puts this TcpDataLink into
/// the "connected" state.
int
OpenDDS::DCPS::TcpDataLink::connect(
  const TcpConnection_rch& connection,
  const TransportSendStrategy_rch& send_strategy,
  const TransportStrategy_rch& receive_strategy)
{
  DBG_ENTRY_LVL("TcpDataLink","connect",6);

  // Sanity check - cannot connect() if we are already connected.
  if (!this->connection_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpDataLink already connected.\n"),
                     -1);
  }

  this->connection_ = connection;

  if (this->connection_->peer().enable(ACE_NONBLOCK) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpDataLink::connect failed to set "
                      "ACE_NONBLOCK %p\n", ACE_TEXT("enable")), -1);
  }

  // Let connection know the datalink for callbacks upon reconnect failure.
  this->connection_->set_datalink(rchandle_from(this));

  // And lastly, inform our base class (DataLink) that we are now "connected",
  // and it should start the strategy objects.
  if (this->start(send_strategy, receive_strategy) != 0) {
    // Our base (DataLink) class failed to start the strategy objects.
    // We need to "undo" some things here before we return -1 to indicate
    // that an error has taken place.

    // Drop our reference to the connection object.
    this->connection_.reset();

    return -1;
  }

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
  if (!this->connection_.is_nil()) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpDataLink::reuse_existing_connection - "
                           "trying to reuse existing connection\n"), 0);
    this->connection_->transfer(connection.in());

    //Connection already exists.
    TransportStrategy_rch brs;
    TransportSendStrategy_rch bss;

    if (this->receive_strategy_.is_nil() && this->send_strategy_.is_nil()) {
      this->connection_.reset();
      return -1;
    } else {
      brs = this->receive_strategy_;
      bss = this->send_strategy_;

      this->connection_ = connection;

      TcpReceiveStrategy* rs = static_cast<TcpReceiveStrategy*>(brs.in());

      TcpSendStrategy* ss = static_cast<TcpSendStrategy*>(bss.in());

      // Associate the new connection object with the receiving strategy and disassociate
      // the old connection object with the receiving strategy.
      int rs_result = rs->reset(this->connection_);

      // Associate the new connection object with the sending strategy and disassociate
      // the old connection object with the sending strategy.
      int ss_result = ss->reset(this->connection_, true);

      if (rs_result == 0 && ss_result == 0) {
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

  // Sanity check - the connection should exist already since we are reconnecting.
  if (this->connection_.is_nil()) {
    VDBG_LVL((LM_ERROR,
              "(%P|%t) ERROR: TcpDataLink::reconnect old connection is nil.\n")
             , 1);
    return -1;
  }

  this->connection_->transfer(connection.in());

  bool released = false;
  TransportStrategy_rch brs;
  TransportSendStrategy_rch bss;

  {
    GuardType guard2(this->strategy_lock_);

    if (this->receive_strategy_.is_nil() && this->send_strategy_.is_nil()) {
      released = true;
      this->connection_.reset();

    } else {
      brs = this->receive_strategy_;
      bss = this->send_strategy_;
    }
  }

  if (released) {
    return this->transport_->connect_tcp_datalink(rchandle_from(this), connection);
  }

  this->connection_ = connection;

  TcpReceiveStrategy* rs = static_cast<TcpReceiveStrategy*>(brs.in());

  TcpSendStrategy* ss = static_cast<TcpSendStrategy*>(bss.in());

  // Associate the new connection object with the receiveing strategy and disassociate
  // the old connection object with the receiveing strategy.
  int rs_result = rs->reset(this->connection_);

  // Associate the new connection object with the sending strategy and disassociate
  // the old connection object with the sending strategy.
  int ss_result = ss->reset(this->connection_);

  if (rs_result == 0 && ss_result == 0) {
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
  //  = OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
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
  ACE_Message_Block* message;
  size_t max_marshaled_size = header_data.max_marshaled_size();
  ACE_Message_Block* data = 0;
  ACE_NEW(data,
          ACE_Message_Block(20,
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

  ACE_NEW(message,
          ACE_Message_Block(max_marshaled_size,
                            ACE_Message_Block::MB_DATA,
                            data, //cont
                            0, //data
                            0, //allocator_strategy
                            0, //locking_strategy
                            ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                            ACE_Time_Value::zero,
                            ACE_Time_Value::max_time,
                            0,
                            0));

  *message << header_data;

  TransportControlElement* send_element = 0;

  ACE_NEW(send_element, TransportControlElement(message));

  // give the message block ownership to TransportControlElement
  message->release();

  // I don't want to rebuild a connection in order to send
  // a graceful disconnect message.
  this->send_i(send_element, false);
}

void OpenDDS::DCPS::TcpDataLink::set_release_pending(bool flag)
{
  this->release_is_pending_ = flag;
}

bool OpenDDS::DCPS::TcpDataLink::is_release_pending() const
{
  return this->release_is_pending_.value();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
