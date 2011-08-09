/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastSession.h"
#include "MulticastSessionFactory.h"
#include "MulticastTransport.h"

#include "ace/Default_Constants.h"
#include "ace/Global_Macros.h"
#include "ace/Log_Msg.h"
#include "ace/Truncate.h"
#include "ace/OS_NS_sys_socket.h"

#include "tao/ORB_Core.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"

#ifndef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* transport,
                                     MulticastSessionFactory* session_factory,
                                     MulticastPeer local_peer,
                                     bool is_loopback,
                                     bool is_active)
  : DataLink(transport, 0, is_loopback, is_active), // priority, is_loopback, is_active
    transport_(transport),
    session_factory_(session_factory, false),
    local_peer_(local_peer),
    config_(0),
    reactor_task_(0)
{
}

MulticastDataLink::~MulticastDataLink()
{
  if (!this->send_buffer_.is_nil()) {
    this->send_strategy_->send_buffer(0);
  }
}

void
MulticastDataLink::configure(MulticastInst* config,
                             TransportReactorTask* reactor_task)
{
  this->config_ = config;
  this->reactor_task_ = reactor_task;
}

void
MulticastDataLink::send_strategy(MulticastSendStrategy* send_strategy)
{
  // A send buffer may be bound to the send strategy to ensure a
  // configured number of most-recent datagrams are retained:
  if (this->session_factory_->requires_send_buffer()) {
    ACE_NEW_NORETURN(this->send_buffer_,
                     TransportSendBuffer(this->config_->nak_depth_,
                                         this->config_->max_samples_per_packet_));
    if (this->send_buffer_.is_nil()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("MulticastDataLink::send_strategy: ")
                 ACE_TEXT("failed to create TransportSendBuffer!\n")));
      return;
    }
    send_strategy->send_buffer(this->send_buffer_.in());
  }
  this->send_strategy_ = send_strategy;
}

void
MulticastDataLink::receive_strategy(MulticastReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

bool
MulticastDataLink::join(const ACE_INET_Addr& group_address)
{
  if (this->socket_.join(group_address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("ACE_SOCK_Dgram_Mcast::join failed.\n")),
                     false);
  }

  ACE_HANDLE handle = this->socket_.get_handle();
  char ttl = this->config_->ttl_;

  if (ACE_OS::setsockopt(handle,
                         IPPROTO_IP,
                         IP_MULTICAST_TTL,
                         &ttl,
                         sizeof(ttl)) < 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("ACE_OS::setsockopt TTL failed.\n")),
                     false);
  }

  int rcv_buffer_size = ACE_Utils::truncate_cast<int>(this->config_->rcv_buffer_size_);
  if (rcv_buffer_size != 0
      && ACE_OS::setsockopt(handle, SOL_SOCKET,
                            SO_RCVBUF,
                            (char *) &rcv_buffer_size,
                            sizeof (int)) < 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("ACE_OS::setsockopt RCVBUF failed.\n")),
                     false);
  }

#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;

  if (ACE_OS::setsockopt(handle, SOL_SOCKET,
                         SO_SNDBUF,
                         (char *) &snd_size,
                         sizeof(snd_size)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("ACE_OS::setsockopt SNDBUF failed to set the send buffer size to %d errno %m\n"),
                      snd_size),
                     false);
  }
#endif /* ACE_DEFAULT_MAX_SOCKET_BUFSIZ */

  if (start(this->send_strategy_.in(), this->recv_strategy_.in()) != 0) {
    this->socket_.close();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("DataLink::start failed!\n")),
                     false);
  }

  return true;
}

MulticastSession*
MulticastDataLink::find_session(MulticastPeer remote_peer)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   0);

  MulticastSessionMap::iterator it(this->sessions_.find(remote_peer));
  if (it != this->sessions_.end()) return it->second.in();
  else return 0;
}

MulticastSession*
MulticastDataLink::find_or_create_session(MulticastPeer remote_peer)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   0);

  MulticastSessionMap::iterator it(this->sessions_.find(remote_peer));
  if (it != this->sessions_.end()) return it->second.in();  // already exists

  MulticastSession_rch session =
    this->session_factory_->create(this, remote_peer);
  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::find_or_create_session: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  std::pair<MulticastSessionMap::iterator, bool> pair = this->sessions_.insert(
    MulticastSessionMap::value_type(remote_peer, session));
  if (pair.first == this->sessions_.end()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::find_or_create_session: ")
                      ACE_TEXT("failed to insert session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  return session._retn();
}

bool
MulticastDataLink::check_header(const TransportHeader& header)
{
  // Skip messages we just sent.

  if (header.source_ == this->local_peer() && ! this->is_loopback_) {
    return false;
  }

  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   false);

  MulticastSessionMap::iterator it(this->sessions_.find(header.source_));
  if (it == this->sessions_.end()) return false;
  if (it->second->acked()) {
    return it->second->check_header(header);
  }

  return true;
}

bool
MulticastDataLink::check_header(const DataSampleHeader& header)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   false);

  if (header.message_id_ == TRANSPORT_CONTROL) return true;

  // Skip data sample unless there is a session for it.
  return (this->sessions_.count(receive_strategy()->received_header().source_) > 0);
}

void
MulticastDataLink::sample_received(ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case TRANSPORT_CONTROL: {
    // Transport control samples are delivered to all sessions
    // regardless of association status:
    {
      ACE_GUARD(ACE_SYNCH_RECURSIVE_MUTEX,
                guard,
                this->session_lock_);

      char* ptr = sample.sample_->rd_ptr();
      for (MulticastSessionMap::iterator it(this->sessions_.begin());
          it != this->sessions_.end(); ++it) {
        it->second->control_received(sample.header_.submessage_id_,
                                     sample.sample_);
        // reset read pointer
        sample.sample_->rd_ptr(ptr);
      }
    }
  } break;

  case SAMPLE_ACK:
    ack_received(sample);
    break;

  default:
    data_received(sample);
  }
}

void
MulticastDataLink::stop_i()
{
  ACE_GUARD(ACE_SYNCH_RECURSIVE_MUTEX,
            guard,
            this->session_lock_);

  for (MulticastSessionMap::iterator it(this->sessions_.begin());
       it != this->sessions_.end(); ++it) {
    it->second->stop();
  }
  this->sessions_.clear();

  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
