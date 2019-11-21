/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpDataLink.h"
#include "UdpTransport.h"
#include "UdpInst.h"
#include "UdpSendStrategy.h"
#include "UdpReceiveStrategy.h"


#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/DirectPriorityMapper.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "UdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

UdpDataLink::UdpDataLink(UdpTransport& transport,
                         Priority   priority,
                         ReactorTask* reactor_task,
                         bool       active)
  : DataLink(transport,
             priority,
             false, // is_loopback,
             active),// is_active
    active_(active),
    reactor_task_(reactor_task),
    send_strategy_(make_rch<UdpSendStrategy>(this)),
    recv_strategy_(make_rch<UdpReceiveStrategy>(this))
{
}

bool
UdpDataLink::open(const ACE_INET_Addr& remote_address)
{
  this->remote_address_ = remote_address;

  UdpInst& config = static_cast<UdpTransport&>(this->impl()).config();

  this->is_loopback_ = this->remote_address_ == config.local_address();

  ACE_INET_Addr local_address;
  if (this->active_) {
    if (local_address.get_type() != remote_address.get_type()) {
      local_address.set(0, "", 0, remote_address.get_type());
    }
  } else {
    local_address = config.local_address();
  }

  if (!open_appropriate_socket_type(this->socket_, local_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("UdpDataLink::open: open_appropriate_socket_type failed\n")),
      false);
  }

  VDBG((LM_DEBUG, "(%P|%t) UdpDataLink::open: listening on %C:%hu\n",
        local_address.get_host_addr(), local_address.get_port_number()));

  // If listening on "any" host/port, need to record the actual port number
  // selected by the OS, as well as our actual hostname, into the config_
  // object's local_address_ for use in UdpTransport::connection_info_i().
  if (!this->active_ && config.local_address().is_any()) {
    ACE_INET_Addr address;
    if (this->socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open - %p"),
                        ACE_TEXT("cannot get local addr\n")), false);
    }
    const unsigned short port = address.get_port_number();
    const std::string hostname = get_fully_qualified_hostname();
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) UdpDataLink::open listening on host %C:%hu\n"),
              hostname.c_str(), port), 2);

    config.local_address(port, hostname.c_str());

  // Similar case to the "if" case above, but with a bound host/IP but no port
  } else if (!this->active_ &&
             0 == config.local_address().get_port_number()) {
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
    config.local_address_set_port(port);
  }

  if (config.send_buffer_size_ > 0) {
    int snd_size = config.send_buffer_size_;
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

  if (config.rcv_buffer_size_ > 0) {
    int rcv_size = config.rcv_buffer_size_;
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

#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  BOOL recv_udp_connreset = FALSE;
  socket_.control(SIO_UDP_CONNRESET, &recv_udp_connreset);
#endif

  if (this->active_) {
    // Set the DiffServ codepoint according to the priority value.
    DirectPriorityMapper mapper(this->transport_priority());
    this->set_dscp_codepoint(mapper.codepoint(), this->socket_);


    // For the active side, send the blob and wait for a 1 byte ack.
    VDBG((LM_DEBUG, "(%P|%t) UdpDataLink::open: active connect to %C:%hu\n",
      remote_address.get_host_addr(), remote_address.get_port_number()));

    TransportLocator info;
    impl().connection_info_i(info, CONNINFO_ALL);
    ACE_Message_Block* data_block;
    ACE_NEW_RETURN(data_block,
                   ACE_Message_Block(info.data.length()+sizeof(Priority),
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
    const ssize_t sent = socket().send(iov, num_blocks, remote_address);
    transport_header_block->release();
    if (sent < 0) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open: ")
                                  ACE_TEXT("failed to send handshake %m\n")),
                       false);
    }

    // Need to wait for the 1 byte ack from the passive side before returning
    // the link (and indicating success).
    const size_t size = 32;
    char buff[size];
    // Default this timeout to 30.  We may want to make this settable
    // or use another settable timeout value here.
    const TimeDuration tv(30);
    const ssize_t recvd = socket().recv(buff, size, this->remote_address_, 0, &tv.value());
    if (recvd == 1) {
      // Expected value
      VDBG_LVL((LM_DEBUG,
                ACE_TEXT("(%P|%t) UdpDataLink::open received handshake ack\n")),
               2);
    } else if (recvd < 0) {
      // Not a handshake ack, something is wrong
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open: ")
                                  ACE_TEXT("failed to receive handshake ack %p\n"),
                        ACE_TEXT("recv")), false);
    } else {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: UdpDataLink::open: ")
                                  ACE_TEXT("failed to receive handshake ack ")
                                  ACE_TEXT("recv returned %b\n"), recvd),
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
  // At this time, the TRANSPORT_CONTROL messages in Udp are only used for
  // the connection handshaking, so receiving one is an indication of the
  // passive_connection event.  In the future the submessage_id_ could be used
  // to allow different types of messages here.
  static_cast<UdpTransport&>(this->impl()).passive_connection(remote_address, sample.sample_);
}

void
UdpDataLink::stop_i()
{
  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
