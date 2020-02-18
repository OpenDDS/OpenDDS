/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpTransport.h"
#include "TcpAcceptor.h"
#include "TcpSendStrategy.h"
#include "TcpReceiveStrategy.h"
#include "TcpInst.h"
#include "TcpDataLink.h"
#include "TcpSynchResource.h"
#include "TcpConnection.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TcpTransport::TcpTransport(TcpInst& inst)
  : TransportImpl(inst)
  , acceptor_(new TcpAcceptor(this))
{
  DBG_ENTRY_LVL("TcpTransport","TcpTransport",6);

  if (!(configure_i(inst) && open())) {
    this->shutdown();
    throw Transport::UnableToCreate();
  }

}

TcpTransport::~TcpTransport()
{
  DBG_ENTRY_LVL("TcpTransport","~TcpTransport",6);
}


TcpInst&
TcpTransport::config() const
{
  return static_cast<TcpInst&>(TransportImpl::config());
}

PriorityKey
TcpTransport::blob_to_key(const TransportBLOB& remote,
                          Priority priority,
                          bool active)
{
  const ACE_INET_Addr remote_address =
    AssociationData::get_remote_address(remote);
  const bool is_loopback = remote_address == config().local_address();
  return PriorityKey(priority, remote_address, is_loopback, active);
}

TransportImpl::AcceptConnectResult
TcpTransport::connect_datalink(const RemoteTransport& remote,
                               const ConnectionAttribs& attribs,
                               const TransportClient_rch& client)
{
  DBG_ENTRY_LVL("TcpTransport", "connect_datalink", 6);

  const PriorityKey key =
    blob_to_key(remote.blob_, attribs.priority_, true /*active*/);

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink PriorityKey "
            "prio=%d, addr=%C:%hu, is_loopback=%d, is_active=%d\n",
            key.priority(), key.address().get_host_addr(),
            key.address().get_port_number(), key.is_loopback(),
            key.is_active()), 0);

  TcpDataLink_rch link;
  {
    GuardType guard(links_lock_);

    if (find_datalink_i(key, link, client, remote.repo_id_)) {
      VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink found datalink link[%@]\n", link.in()), 0);
      return link.is_nil()
        ? AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS)
        : AcceptConnectResult(link);
    }

    link = make_rch<TcpDataLink>(key.address(), ref(*this), attribs.priority_,
                                key.is_loopback(), true /*active*/);
    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink create new link[%@]\n", link.in()), 0);
    if (links_.bind(key, link) != 0 /*OK*/) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TcpTransport::connect_datalink "
                 "Unable to bind new TcpDataLink[%@] to "
                 "TcpTransport in links_ map.\n", link.in()));
      return AcceptConnectResult();
    }
  }

  TcpConnection_rch connection(
    make_rch<TcpConnection>(key.address(), link->transport_priority(), this->config()));
  connection->set_datalink(link);

  TcpConnection* pConn = connection.in();

  ACE_TCHAR str[64];
  key.address().addr_to_string(str,sizeof(str)/sizeof(str[0]), 0);

  // Can't make this call while holding onto TransportClient::lock_
  ACE_Time_Value conn_timeout;
  conn_timeout.msec(this->config().active_conn_timeout_period_);

  const int ret =
    connector_.connect(pConn, key.address(), ACE_Synch_Options(ACE_Synch_Options::USE_REACTOR|ACE_Synch_Options::USE_TIMEOUT, conn_timeout));

  if (ret == -1 && errno != EWOULDBLOCK) {

    VDBG_LVL((LM_ERROR, "(%P|%t) TcpTransport::connect_datalink error %m.\n"), 2);
    ACE_ERROR((LM_ERROR, "(%P|%t) TcpTransport::connect_datalink error %m.\n"));
    //If the connection fails and, in the interim between releasing
    //lock and re-acquiring to remove the failed link, another association may have found
    //the datalink in links_ (always using find_datalink_i) so must allow the other
    //association to either try to connect again (might succeed for it)
    //or try another transport.  If find_datalink_i was called for this datalink, an
    //on_start_callback will be registered and can be invoked.
    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink connect failed, remove link[%@]\n", link.in()), 0);
    {
      GuardType guard(links_lock_);
      if (links_.unbind(key, link) != 0 /*OK*/) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TcpTransport::connect_datalink "
                   "Unable to unbind failed TcpDataLink[%@] from "
                   "TcpTransport links_ map.\n", link.in()));
      }
    }
    link->invoke_on_start_callbacks(false);

    return AcceptConnectResult();
  }

  if (ret == 0) {
    // connect() completed synchronously and called TcpConnection::active_open().
    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink "
              "completed synchronously.\n"), 0);
    return AcceptConnectResult(link);
  }

  if (!link->add_on_start_callback(client, remote.repo_id_)) {
    // link was started by the reactor thread before we could add a callback

    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink got link.\n"), 0);
    return AcceptConnectResult(link);
  }

  add_pending_connection(client, link);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::connect_datalink pending.\n"), 0);
  return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
}

void
TcpTransport::async_connect_failed(const PriorityKey& key)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to make active connection.\n"));
  GuardType guard(links_lock_);
  TcpDataLink_rch link;
  links_.find(key, link);
  links_.unbind(key);
  guard.release();

  if (link.in()) {
    link->invoke_on_start_callbacks(false);
  }
}

//Called with links_lock_ held
bool
TcpTransport::find_datalink_i(const PriorityKey& key, TcpDataLink_rch& link,
                              const TransportClient_rch& client, const RepoId& remote_id)
{
  DBG_ENTRY_LVL("TcpTransport", "find_datalink_i", 6);

  if (links_.find(key, link) == 0 /*OK*/) {
    if (!link->add_on_start_callback(client, remote_id)) {
      VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::find_datalink_i ")
                ACE_TEXT("link[%@] found, already started.\n"), link.in()), 0);
      // Since the link was already started, we won't get an "on start"
      // callback, and the link is immediately usable.
      return true;
    }

    VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::find_datalink_i ")
              ACE_TEXT("link[%@] found, add to pending connections.\n"), link.in()), 0);

    add_pending_connection(client, link);
    link.reset(); // don't return link to TransportClient
    return true;

  } else if (pending_release_links_.find(key, link) == 0 /*OK*/) {
    if (link->cancel_release()) {
      link->set_release_pending(false);

      if (pending_release_links_.unbind(key, link) == 0 /*OK*/
          && links_.bind(key, link) == 0 /*OK*/) {
        VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::find_datalink_i ")
                  ACE_TEXT("found link[%@] in pending release list, cancelled release and moved back to links_.\n"), link.in()), 0);
        return true;
      }
      VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::find_datalink_i ")
                ACE_TEXT("found link[%@] in pending release list but was unable to shift back to links_.\n"), link.in()), 0);
    } else {
      VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::find_datalink_i ")
                ACE_TEXT("found link[%@] in pending release list but was unable to cancel release.\n"), link.in()), 0);
    }
    link.reset(); // don't return link to TransportClient
    return false;
  }

  return false;
}

TransportImpl::AcceptConnectResult
TcpTransport::accept_datalink(const RemoteTransport& remote,
                              const ConnectionAttribs& attribs,
                              const TransportClient_rch& client)
{
  GuidConverter remote_conv(remote.repo_id_);
  GuidConverter local_conv(attribs.local_id_);

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::accept_datalink local %C "
            "accepting connection from remote %C\n",
            std::string(local_conv).c_str(),
            std::string(remote_conv).c_str()), 5);

  const PriorityKey key =
    blob_to_key(remote.blob_, attribs.priority_, false /* !active */);

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::accept_datalink PriorityKey "
            "prio=%d, addr=%C:%hu, is_loopback=%d, is_active=%d\n", attribs.priority_,
            key.address().get_host_addr(), key.address().get_port_number(),
            key.is_loopback(), key.is_active()), 2);

  TcpDataLink_rch link;
  {
    GuardType guard(links_lock_);

    if (find_datalink_i(key, link, client, remote.repo_id_)) {
      return link.is_nil()
        ? AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS)
        : AcceptConnectResult(link);

    } else {
      link = make_rch<TcpDataLink>(key.address(), ref(*this), key.priority(),
                                  key.is_loopback(), key.is_active());

      if (links_.bind(key, link) != 0 /*OK*/) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: TcpTransport::accept_datalink "
                   "Unable to bind new TcpDataLink to "
                   "TcpTransport in links_ map.\n"));
        return AcceptConnectResult();
      }
    }
  }

  TcpConnection_rch connection;
  {
    GuardType guard(connections_lock_);
    const ConnectionMap::iterator iter = connections_.find(key);

    if (iter != connections_.end()) {
      connection = iter->second;
      connections_.erase(iter);
    }
  }

  if (connection.is_nil()) {
    if (!link->add_on_start_callback(client, remote.repo_id_)) {
      VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::accept_datalink "
                "got started link %@.\n", link.in()), 0);
      return AcceptConnectResult(link);
    }

    VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::accept_datalink "
              "no existing TcpConnection.\n"), 0);

    add_pending_connection(client, link);

    // no link ready, passive_connection will complete later
    return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
  }

  if (connect_tcp_datalink(*link, connection) == -1) {
    GuardType guard(links_lock_);
    links_.unbind(key);
    link.reset();
  }

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::accept_datalink "
            "connected link %@.\n", link.in()), 2);
  return AcceptConnectResult(link);
}

void
TcpTransport::stop_accepting_or_connecting(const TransportClient_wrch& client,
                                           const RepoId& remote_id)
{
  GuidConverter remote_converted(remote_id);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport::stop_accepting_or_connecting "
            "stop connecting to remote: %C\n",
            std::string(remote_converted).c_str()), 5);

  GuardType guard(pending_connections_lock_);
  typedef PendConnMap::iterator iter_t;
  const std::pair<iter_t, iter_t> range =
    pending_connections_.equal_range(client);

  for (iter_t iter = range.first; iter != range.second; ++iter) {
    iter->second->remove_on_start_callback(client, remote_id);
  }

  pending_connections_.erase(range.first, range.second);
}

bool
TcpTransport::configure_i(TcpInst& config)
{
  DBG_ENTRY_LVL("TcpTransport", "configure_i", 6);

  this->create_reactor_task();

  connector_.open(reactor_task()->get_reactor());

  // Override with DCPSDefaultAddress.
  if (config.local_address() == ACE_INET_Addr () &&
      !TheServiceParticipant->default_address ().empty ()) {
    config.local_address(0, TheServiceParticipant->default_address ().c_str ());
  }

  // Open our acceptor object so that we can accept passive connections
  // on our config.local_address_.

  if (this->acceptor_->open(config.local_address(),
                            this->reactor_task()->get_reactor()) != 0) {

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Acceptor failed to open %C:%d: %p\n"),
                      config.local_address().get_host_addr(),
                      config.local_address().get_port_number(),
                      ACE_TEXT("open")),
                     false);
  }

  // update the port number (incase port zero was given).
  ACE_INET_Addr address;

  if (this->acceptor_->acceptor().get_local_addr(address) != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpTransport::configure_i ")
               ACE_TEXT("- %p"),
               ACE_TEXT("cannot get local addr\n")));
  }

  OPENDDS_STRING listening_addr(address.get_host_addr());
  VDBG_LVL((LM_DEBUG,
            ACE_TEXT("(%P|%t) TcpTransport::configure_i listening on %C:%hu\n"),
            listening_addr.c_str(), address.get_port_number()), 2);

  unsigned short port = address.get_port_number();

  // As default, the acceptor will be listening on INADDR_ANY but advertise with the fully
  // qualified hostname and actual listening port number.
  if (config.local_address().is_any()) {
    std::string hostname = get_fully_qualified_hostname();
    config.local_address(port, hostname.c_str());
    if (config.local_address() == ACE_INET_Addr()) {
       ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Failed to resolve a local address using fully qualified hostname '%C'\n"),
                          hostname.c_str()),
                          false);
    }
  }

  // Now we got the actual listening port. Update the port number in the configuration
  // if it's 0 originally.
  else if (config.local_address().get_port_number() == 0) {
    config.local_address_set_port(port);
  }

  // Ahhh...  The sweet smell of success!
  return true;
}


void
TcpTransport::shutdown_i()
{
  DBG_ENTRY_LVL("TcpTransport","shutdown_i",6);

  {
    GuardType guard(links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(links_);
         itr.next(entry);
         itr.advance()) {
      entry->int_id_->pre_stop_i();
    }
  }

  // Don't accept any more connections.
  acceptor_->close();
  acceptor_->transport_shutdown();

  {
    {
      GuardType guard(connections_lock_);

      for (ConnectionMap::iterator it = connections_.begin(); it != connections_.end(); ++it) {
        it->second->shutdown();
      }
      connections_.clear();
    }
    {
      GuardType guard(pending_connections_lock_);
      pending_connections_.clear();
    }
  }

  // Disconnect all of our DataLinks, and clear our links_ collection.
  {
    GuardType guard(links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(links_);
         itr.next(entry);
         itr.advance()) {
      entry->int_id_->transport_shutdown();
    }

    links_.unbind_all();

    for (AddrLinkMap::ITERATOR itr(pending_release_links_);
         itr.next(entry);
         itr.advance()) {
      entry->int_id_->transport_shutdown();
    }

    pending_release_links_.unbind_all();
  }

  // Tell our acceptor about this event so that it can drop its reference
  // it holds to this TcpTransport object (via smart-pointer).
  acceptor_->transport_shutdown();
}

bool
TcpTransport::connection_info_i(TransportLocator& local_info, ConnectionInfoFlags flags) const
{
  DBG_ENTRY_LVL("TcpTransport", "connection_info_i", 6);

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport public address str %C\n",
            this->config().get_public_address().c_str()), 2);

  config().populate_locator(local_info, flags);

  return true;
}

void
TcpTransport::release_datalink(DataLink* link)
{
  DBG_ENTRY_LVL("TcpTransport", "release_datalink", 6);

  TcpDataLink* tcp_link = static_cast<TcpDataLink*>(link);

  if (tcp_link == 0) {
    // Really an assertion failure
    ACE_ERROR((LM_ERROR,
               "(%P|%t) INTERNAL ERROR - Failed to downcast DataLink to "
               "TcpDataLink.\n"));
    return;
  }

  TcpDataLink_rch released_link;

  // Possible actions that will be taken to release the link.
  enum LinkAction { None, StopLink, ScheduleLinkRelease };
  LinkAction linkAction = None;

  // Scope for locking to protect the links (and pending_release) containers.
  GuardType guard(this->links_lock_);

  // Attempt to remove the TcpDataLink from our links_ map.
  PriorityKey key(
    tcp_link->transport_priority(),
    tcp_link->remote_address(),
    tcp_link->is_loopback(),
    tcp_link->is_active());

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) TcpTransport::release_datalink link[%@] PriorityKey "
            "prio=%d, addr=%C:%hu, is_loopback=%d, is_active=%d\n",
            link,
            tcp_link->transport_priority(),
            tcp_link->remote_address().get_host_addr(),
            tcp_link->remote_address().get_port_number(),
            (int)tcp_link->is_loopback(),
            (int)tcp_link->is_active()), 2);

  if (this->links_.unbind(key, released_link) != 0) {
    //No op
  } else if (link->datalink_release_delay() > TimeDuration::zero_value) {
    link->set_scheduling_release(true);

    VDBG_LVL((LM_DEBUG,
              "(%P|%t) TcpTransport::release_datalink datalink_release_delay "
              "is %: sec %d usec\n",
              link->datalink_release_delay().value().sec(),
              link->datalink_release_delay().value().usec()), 4);

    // Atomic value update, safe to perform here.
    released_link->set_release_pending(true);

    switch (this->pending_release_links_.bind(key, released_link)) {
    case -1:
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind released TcpDataLink[%@] to "
                 "pending_release_links_ map: %p\n", released_link.in(), ACE_TEXT("bind")));
      linkAction = StopLink;
      break;

    case 1:
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind released TcpDataLink[%@] to "
                 "pending_release_links_ map: already bound\n", released_link.in()));
      linkAction = StopLink;
      break;

    case 0:
      linkAction = ScheduleLinkRelease;
      break;

    default:
      break;
    }

  } else { // datalink_release_delay_ is 0
    link->set_scheduling_release(true);

    linkAction = StopLink;
  }

  // Actions are executed outside of the lock scope.
  switch (linkAction) {
  case StopLink:
    link->schedule_stop(MonotonicTimePoint::now());
    break;

  case ScheduleLinkRelease:
    link->schedule_delayed_release();
    break;

  case None:
    break;
  }

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *link;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::release_datalink() - ")
               ACE_TEXT("link[%@] with priority %d released.\n%C"),
               link,
               link->transport_priority(),
               buffer.str().c_str()));
  }
}

/// This method is called by a TcpConnection object that has been
/// created and opened by our acceptor_ as a result of passively
/// accepting a connection on our local address.  Ultimately, the connection
/// object needs to be paired with a DataLink object that is (or will be)
/// expecting this passive connection to be established.
void
TcpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 const TcpConnection_rch& connection)
{
  DBG_ENTRY_LVL("TcpTransport", "passive_connection", 6);

  const PriorityKey key(connection->transport_priority(),
                        remote_address,
                        remote_address == config().local_address(),
                        connection->is_connector());

  VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TcpTransport::passive_connection() - ")
            ACE_TEXT("established with %C:%d.\n"),
            remote_address.get_host_name(),
            remote_address.get_port_number()), 2);

  GuardType connection_guard(connections_lock_);
  TcpDataLink_rch link;
  {
    GuardType guard(links_lock_);
    links_.find(key, link);
  }

  if (!link.is_nil()) {
    connection_guard.release();

    if (connect_tcp_datalink(*link, connection) == -1) {
      VDBG_LVL((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: connect_tcp_datalink failed\n")), 5);
      GuardType guard(links_lock_);
      links_.unbind(key);

    } else {
      this->fresh_link(connection);
    }

    return;
  }

  // If we reach this point, this link was not in links_, so the
  // accept_datalink() call hasn't happened yet.  Store in connections_ for the
  // accept_datalink() method to find.
  VDBG_LVL((LM_DEBUG, "(%P|%t) # of bef connections: %d\n", connections_.size()), 5);
  const ConnectionMap::iterator where = connections_.find(key);

  if (where != connections_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpTransport::passive_connection() - ")
               ACE_TEXT("connection with %C:%d at priority %d already exists, ")
               ACE_TEXT("overwriting previously established connection.\n"),
               remote_address.get_host_name(),
               remote_address.get_port_number(),
               connection->transport_priority()));
  }

  connections_[key] = connection;
  VDBG_LVL((LM_DEBUG, "(%P|%t) # of after connections: %d\n", connections_.size()), 5);

  this->fresh_link(connection);
}

/// Common code used by accept_datalink(), passive_connection(), and active completion.
int
TcpTransport::connect_tcp_datalink(TcpDataLink& link,
                                   const TcpConnection_rch& connection)
{
  DBG_ENTRY_LVL("TcpTransport", "connect_tcp_datalink", 6);

  if (link.reuse_existing_connection(connection) == 0) {
    return 0;
  }

  ++last_link_;

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::connect_tcp_datalink() [%d] - ")
               ACE_TEXT("creating send strategy with priority %d.\n"),
               last_link_, link.transport_priority()));
  }

  connection->id() = last_link_;

  TcpSendStrategy_rch send_strategy (
    make_rch<TcpSendStrategy>(last_link_, ref(link),
                             new TcpSynchResource(link,
                                                  this->config().max_output_pause_period_),
                             this->reactor_task(), link.transport_priority()));

  TcpReceiveStrategy_rch receive_strategy(
    make_rch<TcpReceiveStrategy>(ref(link), this->reactor_task()));

  if (link.connect(connection, send_strategy, receive_strategy) != 0) {
    return -1;
  }

  return 0;
}

/// This function is called by the TcpReconnectTask thread to check if the passively
/// accepted connection is the re-established connection. If it is, then the "old" connection
/// object in the datalink is replaced by the "new" connection object.
int
TcpTransport::fresh_link(TcpConnection_rch connection)
{
  DBG_ENTRY_LVL("TcpTransport","fresh_link",6);

  TcpDataLink_rch link;
  GuardType guard(this->links_lock_);

  if (is_shut_down()) {
    return 0;
  }

  PriorityKey key(connection->transport_priority(),
                  connection->get_remote_address(),
                  connection->get_remote_address() == this->config().local_address(),
                  connection->is_connector());

  if (this->links_.find(key, link) == 0) {
    TcpConnection_rch old_con = link->get_connection();

    // The connection is accepted but may not be associated with the datalink
    // at this point. The thread calling add_associations() will associate
    // the datalink with the connection in make_passive_connection().
    if (old_con.is_nil()) {
      return 0;
    }

    if (old_con.in() != connection.in())
      // Replace the "old" connection object with the "new" connection object.
    {
      return link->reconnect(connection);
    }
  }

  return 0;
}

void
TcpTransport::unbind_link(DataLink* link)
{
  TcpDataLink* tcp_link = static_cast<TcpDataLink*>(link);

  if (tcp_link == 0) {
    // Really an assertion failure
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpTransport::unbind_link INTERNAL ERROR - "
               "Failed to downcast DataLink to TcpDataLink.\n"));
    return;
  }

  // Attempt to remove the TcpDataLink from our links_ map.
  PriorityKey key(
    tcp_link->transport_priority(),
    tcp_link->remote_address(),
    tcp_link->is_loopback(),
    tcp_link->is_active());

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) TcpTransport::unbind_link link %@ PriorityKey "
            "prio=%d, addr=%C:%hu, is_loopback=%d, is_active=%d\n",
            link,
            tcp_link->transport_priority(),
            tcp_link->remote_address().get_host_addr(),
            tcp_link->remote_address().get_port_number(),
            (int)tcp_link->is_loopback(),
            (int)tcp_link->is_active()), 2);

  GuardType guard(this->links_lock_);

  if (pending_release_links_.unbind(key) && !link->datalink_release_delay().is_zero()) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpTransport::unbind_link INTERNAL ERROR - "
               "Failed to find link %@ tcp_link %@ PriorityKey "
               "prio=%d, addr=%C:%hu, is_loopback=%d, is_active=%d\n",
               link,
               tcp_link,
               tcp_link->transport_priority(),
               tcp_link->remote_address().get_host_addr(),
               tcp_link->remote_address().get_port_number(),
               (int)tcp_link->is_loopback(),
               (int)tcp_link->is_active()));
  }
}


int
TcpTransport::Connector::fini() {
  // Overriding fini() so that  ACE_Connector<TcpConnection, ACE_SOCK_Connector>::close() won't be
  // invoked in the process shutting down reactor. Without overrinding fini(), close() would be called
  // from destructor and from reactor in different threads which leads to synchronization issues.
  return 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
