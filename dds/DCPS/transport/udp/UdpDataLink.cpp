/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpDataLink.h"
#include "UdpTransport.h"
#include "UdpInst.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "UdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

UdpDataLink::UdpDataLink(UdpTransport* transport,
                         bool active)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             active),// is_active
    active_(active),
    config_(0),
    reactor_task_(0)
{
}

bool
UdpDataLink::open(const ACE_INET_Addr& remote_address)
{
  this->remote_address_ = remote_address;
  this->is_loopback_ = this->remote_address_ == this->config_->local_address_;

  ACE_INET_Addr local_address;
  if (!this->active_) {
    local_address = this->config_->local_address_;
  }

  if (this->socket_.open(local_address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: open failed: %m\n")),
                     false);
  }

  // If listening on "any" host/port, need to record the actual port number
  // selected by the OS, as well as our actual hostname, into the config_
  // object's local_address_ for use in UdpTransport::connection_info_i().
  if (!this->active_ && this->config_->local_address_.is_any()) {
    ACE_INET_Addr address;
    if (this->socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open - %p"),
                        ACE_TEXT("cannot get local addr\n")), false);
    }
    const unsigned short port = address.get_port_number();
    const std::string hostname = get_fully_qualified_hostname();
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) UdpDataLink::open listening on %C:%hu\n"),
              hostname.c_str(), port), 2);
    this->config_->local_address_.set(port, hostname.c_str());

  // Similar case to the "if" case above, but with a bound host/IP but no port
  } else if (!this->active_ &&
             0 == this->config_->local_address_.get_port_number()) {
    ACE_INET_Addr address;
    if (this->socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open - %p"),
                        ACE_TEXT("cannot get local addr\n")), false);
    }
    const unsigned short port = address.get_port_number();
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) UdpDataLink::open listening on port %hu\n"),
              port), 2);
    this->config_->local_address_.set_port_number(port);
  }

  if (this->config_->send_buffer_size_ > 0) {
    int snd_size = this->config_->send_buffer_size_;
    if (this->socket_.set_option(SOL_SOCKET,
                                SO_SNDBUF,
                                (void *) &snd_size,
                                sizeof(snd_size)) < 0
        && errno != ENOTSUP) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("UdpDataLink::open: failed to set the send buffer size to %d errno %m\n"),
                        snd_size),
                       false);
    }
  }

  if (this->config_->send_buffer_size_ > 0) {
    int rcv_size = this->config_->rcv_buffer_size_;
    if (this->socket_.set_option(SOL_SOCKET,
                                SO_RCVBUF,
                                (void *) &rcv_size,
                                sizeof(int)) < 0
        && errno != ENOTSUP) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("UdpDataLink::open: failed to set the receive buffer size to %d errno %m \n"),
                        rcv_size),
                       false);
    }
  }

  if (this->active_) {
    // For the active side, send the blob and wait for a 1 byte ack.

    TransportLocator info;
    this->impl()->connection_info_i(info);
    ACE_Message_Block* data_block;
    ACE_NEW_RETURN(data_block,
                   ACE_Message_Block(info.data.length()+sizeof(CORBA::Long),
                                     ACE_Message_Block::MB_DATA,
                                     0, //cont
                                     0, //data
                                     0, //allocator_strategy
                                     0, //locking_strategy
                                     ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                     ACE_Time_Value::zero,
                                     ACE_Time_Value::max_time,
                                     0,
                                     0),
                   0);

    Serializer serializer(data_block);
    serializer << this->transport_priority();
    serializer.write_octet_array(info.data.get_buffer(),
                                 info.data.length());

    DataSampleHeader sample_header;
    sample_header.message_id_ = TRANSPORT_CONTROL;
    sample_header.message_length_ =
      static_cast<ACE_UINT32>(data_block->length());
    ACE_Message_Block* sample_header_block;
    ACE_NEW_RETURN(sample_header_block,
                   ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                     ACE_Message_Block::MB_DATA,
                                     0, //cont
                                     0, //data
                                     0, //allocator_strategy
                                     0, //locking_strategy
                                     ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                     ACE_Time_Value::zero,
                                     ACE_Time_Value::max_time,
                                     0,
                                     0),
                   0);
    *sample_header_block << sample_header;
    sample_header_block->cont(data_block);

    ACE_Message_Block* transport_header_block;
    TransportHeader transport_header;
    ACE_NEW_RETURN(transport_header_block,
                   ACE_Message_Block(TransportHeader::max_marshaled_size(),
                                     ACE_Message_Block::MB_DATA,
                                     0,
                                     0,
                                     0,
                                     0,
                                     ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                     ACE_Time_Value::zero,
                                     ACE_Time_Value::max_time,
                                     0,
                                     0),
                   0);

    transport_header.length_ =
      static_cast<ACE_UINT32>(data_block->length() +
                              sample_header_block->length());
    *transport_header_block << transport_header;
    transport_header_block->cont(sample_header_block);

    iovec iov[MAX_SEND_BLOCKS];
    const int num_blocks =
      TransportSendStrategy::mb_to_iov(*transport_header_block, iov);
    this->socket().send(iov, num_blocks, remote_address);
    transport_header_block->release();

    // Need to wait for the 1 byte ack from the passive side before returning
    // the link (and indicating success).
    const size_t size = 32;
    char buff[size];
    // Default this timeout to 30.  We may want to make this settable
    // or use another settable timeout value here.
    ACE_Time_Value tv(30);
    if (this->socket().recv(buff, size, this->remote_address_, 0, &tv) == 1) {
      // Expected value
      VDBG_LVL((LM_DEBUG,
                ACE_TEXT("(%P|%t) UdpDataLink::open received handshake ack\n")),
               2);
    } else {
      // Not a handshake ack, something is wrong
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("UdpDataLink::open: failed to receive handshake ack\n")),
                       false);
    }
  }

  if (start(static_rchandle_cast<TransportSendStrategy>(this->send_strategy_),
            static_rchandle_cast<TransportStrategy>(this->recv_strategy_))
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
UdpDataLink::control_received(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address)
{
  TransportImpl_rch impl = this->impl();
  RcHandle<UdpTransport> ut = static_rchandle_cast<UdpTransport>(impl);
  // At this time, the TRANSPORT_CONTROL messages in Udp are only used for
  // the connection handshaking, so receiving one is an indication of the
  // passive_connection event.  In the future the submessage_id_ could be used
  // to allow different types of messages here.
  ut->passive_connection(remote_address, sample.sample_);
}

void
UdpDataLink::stop_i()
{
  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
