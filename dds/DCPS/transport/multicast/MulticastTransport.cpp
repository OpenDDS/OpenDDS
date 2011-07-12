/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"
#include "MulticastDataLink.h"
#include "MulticastReceiveStrategy.h"
#include "MulticastSendStrategy.h"
#include "MulticastSession.h"
#include "BestEffortSessionFactory.h"
#include "ReliableSessionFactory.h"

#include "ace/Log_Msg.h"
#include "ace/Truncate.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4d435354); // MCST

} // namespace

namespace OpenDDS {
namespace DCPS {

MulticastTransport::MulticastTransport(const TransportInst_rch& inst)
  : config_i_(0)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

MulticastTransport::~MulticastTransport()
{
}

DataLink*
MulticastTransport::find_datalink(
  RepoId local_id,
  const AssociationData& remote_association,
  CORBA::Long priority,
  bool active)
{
  // To accommodate the one-to-many nature of multicast reservations,
  // a session layer is used to maintain state between unique pairs
  // of DomainParticipants over a single DataLink instance. Given
  // that TransportImpl instances may only be attached to either
  // Subscribers or Publishers within the same DomainParticipant,
  // it may be assumed that the local_id always references the same
  // participant.
  MulticastDataLink_rch link;
  if (active && !this->client_link_.is_nil()) {
    link = this->client_link_;
  }

  if (!active && !this->server_link_.is_nil()) {
    link = this->server_link_;
  }

  if (!link.is_nil()) {

    MulticastPeer remote_peer =
      RepoIdConverter(remote_association.remote_id_).participantId();

    MulticastSession_rch session = link->find_or_create_session(remote_peer);

    if (session.is_nil()) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("MulticastTransport::find_datalink: ")
                        ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                        remote_peer),
                       0);
    }

    if (!session->start(active)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("MulticastTransport::find_datalink: ")
                        ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                        remote_peer),
                       0);
    }
  }

  return link._retn();
}

DataLink*
MulticastTransport::create_datalink(
  RepoId local_id,
  const AssociationData& remote_association,
  CORBA::Long priority,
  bool active)
{
  RcHandle<MulticastSessionFactory> session_factory;
  if (this->config_i_->reliable_) {
    ACE_NEW_RETURN(session_factory, ReliableSessionFactory, 0);
  } else {
    ACE_NEW_RETURN(session_factory, BestEffortSessionFactory, 0);
  }

  MulticastPeer local_peer = RepoIdConverter(local_id).participantId();
  MulticastPeer remote_peer =
    RepoIdConverter(remote_association.remote_id_).participantId();

  bool is_loopback = local_peer == remote_peer;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport::create_datalink remote addr str "
            "\"%s\" priority %d is_loopback %d active %d\"\n",
            remote_association.network_order_address_.addr_.c_str(),
            priority, is_loopback, active),
            2);


  MulticastDataLink_rch link;
  ACE_NEW_RETURN(link,
                 MulticastDataLink(this,
                                   session_factory.in(),
                                   local_peer,
                                   is_loopback,
                                   active),
                 0);

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send strategy:
  MulticastSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, MulticastSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  MulticastReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, MulticastReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Join multicast group:
  if (!link->join(this->config_i_->group_address_)) {
    ACE_TCHAR str[64];
    this->config_i_->group_address_.addr_to_string(str, sizeof(str));
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to join multicast group: %C!\n"),
                      str),
                     0);
  }

  (active ? this->client_link_ : this->server_link_) = link;

  MulticastSession_rch session = link->find_or_create_session(remote_peer);
  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  if (!session->start(active)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticasTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  return link._retn();
}

int
MulticastTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<MulticastInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     -1);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  return 0;
}

void
MulticastTransport::shutdown_i()
{
  if (!this->client_link_.is_nil()) {
    this->client_link_->transport_shutdown();
  }
  if (!this->server_link_.is_nil()) {
    this->server_link_->transport_shutdown();
  }
  this->config_i_ = 0;
}

int
MulticastTransport::connection_info_i(TransportInterfaceInfo& info) const
{
  NetworkAddress network_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  size_t len = cdr.total_length();
  char *buffer = const_cast<char*>(cdr.buffer()); // safe

  // Provide connection information for endpoint identification by
  // the DCPSInfoRepo. These values are not used by multicast
  // for DataLink establishment.
  info.transport_id = TRANSPORT_INTERFACE_ID;

  info.data = TransportInterfaceBLOB(ACE_Utils::truncate_cast<CORBA::ULong>(len),
                                     ACE_Utils::truncate_cast<CORBA::ULong>(len),
    reinterpret_cast<CORBA::Octet*>(buffer));

  info.publication_transport_priority = 0;

  return 0;
}

bool
MulticastTransport::acked(RepoId /*local_id*/, RepoId remote_id)
{
  bool is_client = ! (this->client_link_.is_nil());
  bool is_server = ! (this->server_link_.is_nil());
  bool acked = false;

  if (is_client || is_server) {
    MulticastPeer remote_peer =
      RepoIdConverter(remote_id).participantId();

     if (is_client) {
       acked = acked || this->client_link_->acked(remote_peer);
     }
     if (is_server) {
       acked = acked || this->server_link_->acked(remote_peer);
     }

    return acked;
  }

  return false;
}

void
MulticastTransport::remove_ack(RepoId /*local_id*/, RepoId /*remote_id*/)
{
  // Association acks are managed by our MulticastDataLink; there
  // is no state that needs to be removed in MulticastTransport.
}

void
MulticastTransport::release_datalink_i(DataLink* /*link*/,
                                       bool /*release_pending*/)
{
  this->client_link_ = 0;  // release ownership
  this->server_link_ = 0;  // release ownership
}

} // namespace DCPS
} // namespace OpenDDS
