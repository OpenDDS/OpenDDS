// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpReceiveStrategy.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpSendStrategy.h"
#include  "dds/DCPS/transport/framework/TransportControlElement.h"
#include  "dds/DCPS/DataSampleHeader.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpDataLink::SimpleTcpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         TAO::DCPS::SimpleTcpTransport*  transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address),
    graceful_disconnect_sent_ (false)
{
  DBG_ENTRY("SimpleTcpDataLink","SimpleTcpDataLink");
  transport_impl->_add_ref ();
  this->transport_ = transport_impl;
}


TAO::DCPS::SimpleTcpDataLink::~SimpleTcpDataLink()
{
  DBG_ENTRY("SimpleTcpDataLink","~SimpleTcpDataLink");
}


/// Called when the DataLink has been "stopped" for some reason.  It could
/// be called from the DataLink::transport_shutdown() method (when the
/// TransportImpl is handling a shutdown() call).  Or, it could be called
/// from the DataLink::release_reservations() method, when it discovers that
/// it has just released the last remaining reservations from the DataLink,
/// and the DataLink is in the process of "releasing" itself.
void
TAO::DCPS::SimpleTcpDataLink::stop_i()
{
  DBG_ENTRY("SimpleTcpDataLink","stop_i");

  if (!this->connection_.is_nil())
    {      
      // Tell the connection object to disconnect.
      this->connection_->disconnect();

      // Drop our reference to the connection object.
      this->connection_ = 0;
    }
}


void
TAO::DCPS::SimpleTcpDataLink::pre_stop_i()
{
  DBG_ENTRY("SimpleTcpDataLink","stop_i");
 
  DataLink::pre_stop_i();

  SimpleTcpReceiveStrategy * rs 
    = dynamic_cast <SimpleTcpReceiveStrategy*> (this->receive_strategy_.in ());

  // If we received the GRACEFUL_DISCONNECT message from peer before we
  // initiate the disconnecting of the datalink, then we will not send 
  // the GRACEFUL_DISCONNECT message to the peer.
  bool disconnected = rs->gracefully_disconnected ();

  if (!this->connection_.is_nil() && !this->graceful_disconnect_sent_
    && ! disconnected)
    {
      this->send_graceful_disconnect_message ();
      this->graceful_disconnect_sent_ = true;
    }
  if (!this->connection_.is_nil())
    {
      this->connection_->shutdown ();
    }
}

/// The SimpleTcpTransport calls this method when it has an established
/// connection object for us.  This call puts this SimpleTcpDataLink into
/// the "connected" state.
int
TAO::DCPS::SimpleTcpDataLink::connect
                                 (SimpleTcpConnection*      connection,
                                  TransportSendStrategy*    send_strategy,
                                  TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY("SimpleTcpDataLink","connect");

  // Sanity check - cannot connect() if we are already connected.
  if (!this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpDataLink already connected.\n"),
                       -1);
    }

  // Keep a "copy" of the reference to the connection object for ourselves.
  connection->_add_ref();
  this->connection_ = connection;

  // Let connection know the datalink for callbacks upon reconnect failure.
  this->connection_->set_datalink (this);

  // And lastly, inform our base class (DataLink) that we are now "connected",
  // and it should start the strategy objects.
  if (this->start(send_strategy,receive_strategy) != 0)
    {
      // Our base (DataLink) class failed to start the strategy objects.
      // We need to "undo" some things here before we return -1 to indicate
      // that an error has taken place.

      // Drop our reference to the connection object.
      this->connection_ = 0;

      return -1;
    }

  return 0;
}


/// Associate the new connection object with this datalink object.
/// The states of the "old" connection object are copied to the new 
/// connection object and the "old" connection object is replaced by 
/// the new connection object.
int
TAO::DCPS::SimpleTcpDataLink::reconnect (SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpDataLink","reconnect");

  // Sanity check - the connection should exist already since we are reconnecting.
  if (this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpDataLink::reconnect old connection is nil.\n"),
                       -1);
    }

  // Keep a "copy" of the reference to the connection object for ourselves.
  connection->_add_ref();
  this->connection_->transfer (connection);
  this->connection_ = connection;

  SimpleTcpReceiveStrategy* rs 
    = dynamic_cast <SimpleTcpReceiveStrategy*> (this->receive_strategy_.in ());

  if (rs == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) ERROR: SimpleTcpDataLink::reconnect dynamic_cast failed\n"),
        -1);
    }
  // Associate the new connection object with the receiveing strategy and disassociate
  // the old connection object with the receiveing strategy.
  return rs->reset (this->connection_.in ());
}



void 
TAO::DCPS::SimpleTcpDataLink::send_graceful_disconnect_message ()
{
  DBG_ENTRY("SimpleTcpDataLink","send_graceful_disconnect_message");
  DataSampleHeader header_data; 
  // The message_id_ is the most important value for the DataSampleHeader.
  header_data.message_id_ = GRACEFUL_DISCONNECT;

  // Other data in the DataSampleHeader are not necessary set. The bogus values
  // can be used.

  //header_data.byte_order_ 
  //  = this->transport_->get_configuration()->swap_bytes() ? !TAO_ENCAP_BYTE_ORDER : TAO_ENCAP_BYTE_ORDER;
  //header_data.message_length_ = 0;
  //header_data.sequence_ = 0;
  //::DDS::Time_t source_timestamp 
  //  = ::TAO::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
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
  size_t max_marshaled_size = header_data.max_marshaled_size ();
  ACE_Message_Block* data = 0;
  ACE_NEW (data,
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
  data->wr_ptr (20);

  header_data.message_length_ = data->length ();

  ACE_NEW (message,
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

  message << header_data;    

  TransportControlElement* send_element = 0;

  ACE_NEW(send_element, TransportControlElement(message));

  this->send_i (send_element);

  message->release ();

  delete send_element;
}


void 
TAO::DCPS::SimpleTcpDataLink::fully_associated ()
{
  DBG_ENTRY("SimpleTcpDataLink","fully_associated");

  bool swap_byte = this->transport_->get_configuration()->swap_bytes_;
  DataSampleHeader header_data; 
  // The message_id_ is the most important value for the DataSampleHeader.
  header_data.message_id_ = FULLY_ASSOCIATED;

  // Other data in the DataSampleHeader are not necessary set. The bogus values
  // can be used.

  header_data.byte_order_ 
    = swap_byte ? !TAO_ENCAP_BYTE_ORDER : TAO_ENCAP_BYTE_ORDER;
  //header_data.message_length_ = 0;
  //header_data.sequence_ = 0;
  //::DDS::Time_t source_timestamp 
  //  = ::TAO::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
  //header_data.source_timestamp_sec_ = source_timestamp.sec;
  //header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;
  //header_data.coherency_group_ = 0;
  //header_data.publication_id_ = 0;

  ACE_Message_Block* message;
  size_t max_marshaled_size = header_data.max_marshaled_size ();

  ACE_Message_Block* data = this->marshal_acks (swap_byte);

  header_data.message_length_ = data->length ();

  ACE_NEW (message,
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
  
  message << header_data;   

  TransportControlElement* send_element = 0;

  ACE_NEW(send_element, TransportControlElement(message));

  this->send_i (send_element);

  message->release ();

  delete send_element;
}







