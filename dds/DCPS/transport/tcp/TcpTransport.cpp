/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpTransport.h"
#include "TcpConnectionReplaceTask.h"
#include "TcpAcceptor.h"
#include "TcpSendStrategy.h"
#include "TcpReceiveStrategy.h"
#include "TcpInst.h"
#include "TcpDataLink.h"
#include "TcpSynchResource.h"
#include "TcpConnection.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/debug.h"
#include <sstream>

namespace OpenDDS {
namespace DCPS {

TcpTransport::TcpTransport(const TransportInst_rch& inst)
  : reverse_reservation_lock_(this->reservation_lock()),
    acceptor_(new TcpAcceptor(this)),
    connections_updated_(this->connections_lock_),
    con_checker_(new TcpConnectionReplaceTask(this))
{
  DBG_ENTRY_LVL("TcpTransport","TcpTransport",6);
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

TcpTransport::~TcpTransport()
{
  DBG_ENTRY_LVL("TcpTransport","~TcpTransport",6);
  delete acceptor_;

  con_checker_->close(1);  // This could potentially fix a race condition
  delete con_checker_;
}

PriorityKey
TcpTransport::blob_to_key(const TransportBLOB& remote,
                          CORBA::Long priority,
                          bool active)
{
  ACE_INET_Addr remote_address = AssociationData::get_remote_address(remote);

  const bool is_loopback = remote_address == this->tcp_config_->local_address_;

  return PriorityKey(priority, remote_address, is_loopback, active);
}

DataLink*
TcpTransport::find_datalink_i(const RepoId& /*local_id*/,
                              const RepoId& /*remote_id*/,
                              const TransportBLOB& remote_data,
                              CORBA::Long priority,
                              bool active)
{
  DBG_ENTRY_LVL("TcpTransport", "find_datalink_i", 6);

  const PriorityKey key = this->blob_to_key(remote_data, priority, active);

  VDBG_LVL((LM_DEBUG,
            ACE_TEXT("(%P|%t) TcpTransport::find_datalink ")
            ACE_TEXT("remote_address \"%C:%d priority %d ")
            ACE_TEXT("is_loopback %d\"\n"),
            key.address().get_host_name(),
            key.address().get_port_number(),
            priority, key.is_loopback()),
          2);

  TcpDataLink_rch link;
  GuardType guard(this->links_lock_);

  // First, we have to try to find an existing (connected) DataLink
  // that suits the caller's needs.

  if (this->links_.find(key, link) == 0) {

    link->wait_for_start();

    TcpConnection_rch con = link->get_connection();

    if (con.is_nil()) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to use found datalink for "
                        "remote %C:%d.\n",
                        key.address().get_host_addr(),
                        key.address().get_port_number()),
                       0);
    }

    if (con->is_connector() && !con->is_connected()) {
      bool on_new_association = true;

      if (con->reconnect(on_new_association) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "(%P|%t) ERROR: Unable to reconnect to remote %C:%d.\n",
                          key.address().get_host_addr(),
                          key.address().get_port_number()),
                         0);
      }
    }

    // This means we may or may not find a suitable (and already connected) DataLink.
    // Thus we need more checks.
    else {
      if (!con->is_connector() && !con->is_connected()) {
        // The passive connecting side will wait for the connection establishment.
      }

    }

    if (DCPS_debug_level >= 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Found existing connection,"
        " No need for passive connection establishment, transport: %C.\n",
        this->config()->name().c_str()));
    }
    return link._retn();

  } else if (this->pending_release_links_.find(key, link) == 0) {
    if (link->cancel_release()) {
      link->set_release_pending(false);
      if (this->pending_release_links_.unbind(key, link) == 0 && this->links_.bind(key, link) == 0) {
        VDBG_LVL((LM_DEBUG, "(%P|%t) Move link prio=%d addr=%C:%d to links_\n",
                  link->transport_priority(), link->remote_address().get_host_name(),
                  link->remote_address().get_port_number()), 5);
        return link._retn();

      } else {
        // This should not happen.
        ACE_ERROR((LM_ERROR, "(%P|%t) Failed to move link prio=%d addr=%C:%d to links_\n",
                   link->transport_priority(), link->remote_address().get_host_name(),
                   link->remote_address().get_port_number()));
      }
    }
  }

  return 0;
}

DataLink*
TcpTransport::connect_datalink_i(const RepoId& local_id,
                                 const RepoId& /*remote_id*/,
                                 const TransportBLOB& remote_data,
                                 CORBA::Long priority)
{
  DBG_ENTRY_LVL("TcpTransport", "connect_datalink_i", 6);

  const PriorityKey key =
    this->blob_to_key(remote_data, priority, true /*active*/);

  TcpDataLink_rch link =
    new TcpDataLink(key.address(), this, priority,
                    key.is_loopback(), true /*active*/);

  { // guard scope
    GuardType guard(this->links_lock_);

    // Attempt to bind the TcpDataLink to our links_ map.
    if (this->links_.bind(key, link) != 0) {
      // We failed to bind the new DataLink into our links_ map.
      // On error, we return a NULL pointer.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TcpTransport::connect_datalink_i "
                        "Unable to bind new TcpDataLink to "
                        "TcpTransport in links_ map.\n"), 0);
    }
  }

  // Now we need to attempt to establish a connection for the DataLink.
  int result = this->make_active_connection(key.address(), link.in());

  if (result != 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: Failed to make active connection.\n"));

    link->unblock_wait_for_start();

    GuardType guard(this->links_lock_);
    // Make sure that we unbind the link (that failed to establish a
    // connection) from our links_ map.  We intentionally ignore the
    // return code from the unbind() call since we know that we just
    // did the bind() moments ago - and with the links_lock_ acquired
    // the whole time.
    this->links_.unbind(key);

    // On error, return a NULL pointer.
    return 0;
  }

  // That worked.  Return a reference to the DataLink that the caller will
  // be responsible for.
  return link._retn();
}

DataLink*
TcpTransport::accept_datalink(TransportImpl::ConnectionEvent& ce)
{
  const std::string ttype = "tcp";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();

  std::vector<PriorityKey> keys;
  GuardType guard(this->connections_lock_);

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {

      const PriorityKey key =
        this->blob_to_key(ce.remote_association_.remote_data_[idx].data,
                          ce.priority_, false /*active == false*/);

      TcpDataLink_rch link = 
        new TcpDataLink(key.address(), this, ce.priority_,
                        key.is_loopback(), false /*active == false*/);
      {
        GuardType guard(this->links_lock_);
        if (this->links_.bind(key, link) != 0) {
          this->unbind_all(keys, &guard);
          ACE_ERROR_RETURN((LM_ERROR,
                            "(%P|%t) ERROR: TcpTransport::accept_datalink "
                            "Unable to bind new TcpDataLink to "
                            "TcpTransport in links_ map.\n"), 0);
        }
      }

      ConnectionMap::iterator iter = this->connections_.find(key);
      if (iter != this->connections_.end()) {
        TcpConnection_rch connection = iter->second;
        this->connections_.erase(iter);

        if (this->connect_tcp_datalink(link.in(), connection.in()) == -1) {
          GuardType guard(this->links_lock_);
          this->links_.unbind(key);
          link = 0;
        }
        this->unbind_all(keys); // these are the blobs that didn't connect
        return link._retn();
      }
      keys.push_back(key);
    }
  }

  for (size_t i = 0; i < keys.size(); ++i) {
    this->pending_connections_.insert(std::make_pair(&ce, keys[i]));
  }

  return 0;
}

void
TcpTransport::unbind_all(const std::vector<PriorityKey>& keys)
{
  GuardType guard(this->links_lock_);
  this->unbind_all(keys, &guard);
}

void
TcpTransport::unbind_all(const std::vector<PriorityKey>& keys, GuardType*)
{
  for (size_t i = 0; i < keys.size(); ++i) {
    this->links_.unbind(keys[i]);
  }
}

void
TcpTransport::stop_accepting(TransportImpl::ConnectionEvent& ce)
{
  GuardType guard(this->connections_lock_);
  typedef std::multimap<ConnectionEvent*, PriorityKey>::iterator iter_t;
  std::pair<iter_t, iter_t> range = this->pending_connections_.equal_range(&ce);
  for (iter_t iter = range.first; iter != range.second; ++iter) {
    GuardType guard(this->links_lock_);
    this->links_.unbind(iter->second);
  }
  this->pending_connections_.erase(range.first, range.second);
}

bool
TcpTransport::configure_i(TransportInst* config)
{
  DBG_ENTRY_LVL("TcpTransport", "configure_i", 6);

  // Downcast the config argument to a TcpInst*
  TcpInst* tcp_config =
    static_cast<TcpInst*>(config);

  if (tcp_config == 0) {
    // The downcast failed.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Failed downcast from TransportInst "
                      "to TcpInst.\n"),
                     false);
  }

  this->create_reactor_task();

  // Ask our base class for a "copy" of the reference to the reactor task.
  this->reactor_task_ = reactor_task();

  // Make a "copy" of the reference for ourselves.
  tcp_config->_add_ref();
  this->tcp_config_ = tcp_config;

  // Open the reconnect task
  if (this->con_checker_->open()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: connection checker failed to open : %p\n"),
                      ACE_TEXT("open")),
                     false);
  }

  // Open our acceptor object so that we can accept passive connections
  // on our this->tcp_config_->local_address_.

  if (this->acceptor_->open(this->tcp_config_->local_address_,
                            this->reactor_task_->get_reactor()) != 0) {
    // Remember to drop our reference to the tcp_config_ object since
    // we are about to return -1 here, which means we are supposed to
    // keep a copy after all.
    TcpInst_rch cfg = this->tcp_config_._retn();

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Acceptor failed to open %C:%d: %p\n"),
                      cfg->local_address_.get_host_addr(),
                      cfg->local_address_.get_port_number(),
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

  VDBG_LVL((LM_DEBUG,
            ACE_TEXT("(%P|%t) TcpTransport::configure_i listening on %C:%hu\n"),
            address.get_host_name(), address.get_port_number()), 2);

  unsigned short port = address.get_port_number();
  std::stringstream out;
  out << port;

  // As default, the acceptor will be listening on INADDR_ANY but advertise with the fully
  // qualified hostname and actual listening port number.
  if (tcp_config_->local_address_.is_any()) {
    std::string hostname = get_fully_qualified_hostname();

    this->tcp_config_->local_address_.set(port, hostname.c_str());
    this->tcp_config_->local_address_str_ = hostname;
    this->tcp_config_->local_address_str_ += ':' + out.str();
  }

  // Now we got the actual listening port. Update the port nnmber in the configuration
  // if it's 0 originally.
  else if (tcp_config_->local_address_.get_port_number() == 0) {
    this->tcp_config_->local_address_.set_port_number(port);

    if (this->tcp_config_->local_address_str_.length() > 0) {
      size_t pos = this->tcp_config_->local_address_str_.find(':');
      std::string str = this->tcp_config_->local_address_str_.substr(0, pos + 1);
      str += out.str();
      this->tcp_config_->local_address_str_ = str;
    }
  }

  // Ahhh...  The sweet smell of success!
  return true;
}

void
TcpTransport::pre_shutdown_i()
{
  DBG_ENTRY_LVL("TcpTransport","pre_shutdown_i",6);

  GuardType guard(this->links_lock_);

  AddrLinkMap::ENTRY* entry;

  for (AddrLinkMap::ITERATOR itr(this->links_);
       itr.next(entry);
       itr.advance()) {
    entry->int_id_->pre_stop_i();
  }
}

void
TcpTransport::shutdown_i()
{
  DBG_ENTRY_LVL("TcpTransport","shutdown_i",6);

  // Don't accept any more connections.
  this->acceptor_->close();
  this->acceptor_->transport_shutdown();

  this->con_checker_->close(1);

  {
    GuardType guard(this->connections_lock_);

    this->connections_.clear();
    // TBD SOON - Need to set some flag to tell those threads waiting on
    //            the connections_updated_ condition that there is no hope
    //            of ever getting the connection that they seek - they should
    //            give up rather than continue to wait.

    // We need to signal all of the threads that may be stuck wait()'ing
    // on the connections_updated_ condition.
    this->connections_updated_.broadcast();
  }

  // Disconnect all of our DataLinks, and clear our links_ collection.
  {
    GuardType guard(this->links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(this->links_);
         itr.next(entry);
         itr.advance()) {
      entry->int_id_->transport_shutdown();
    }

    this->links_.unbind_all();

    for (AddrLinkMap::ITERATOR itr(this->pending_release_links_);
         itr.next(entry);
         itr.advance()) {
      entry->int_id_->transport_shutdown();
    }

    this->pending_release_links_.unbind_all();
  }

  // Drop our reference to the TcpInst object.
  this->tcp_config_ = 0;

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

  // Tell our acceptor about this event so that it can drop its reference
  // it holds to this TcpTransport object (via smart-pointer).
  this->acceptor_->transport_shutdown();
}

bool
TcpTransport::connection_info_i(TransportLocator& local_info) const
{
  DBG_ENTRY_LVL("TcpTransport", "connection_info_i", 6);

  VDBG_LVL((LM_DEBUG, "(%P|%t) TcpTransport local address str %C\n",
            this->tcp_config_->local_address_str_.c_str()), 2);

  //Always use local address string to provide to DCPSInfoRepo for advertisement.
  NetworkAddress network_order_address(this->tcp_config_->local_address_str_);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  local_info.transport_type = "tcp";
  local_info.data = TransportBLOB(len, len,
                                  reinterpret_cast<CORBA::Octet*>(buffer));
  return true;
}

void
TcpTransport::release_datalink_i(DataLink* link, bool release_pending)
{
  DBG_ENTRY_LVL("TcpTransport", "release_datalink_i", 6);

  TcpDataLink* tcp_link = static_cast<TcpDataLink*>(link);

  if (tcp_link == 0) {
    // Really an assertion failure
    ACE_ERROR((LM_ERROR,
               "(%P|%t) INTERNAL ERROR - Failed to downcast DataLink to "
               "TcpDataLink.\n"));
    return;
  }

  TcpDataLink_rch released_link;

  GuardType guard(this->links_lock_);

  // Attempt to remove the TcpDataLink from our links_ map.
  PriorityKey key(
    tcp_link->transport_priority(),
    tcp_link->remote_address(),
    tcp_link->is_loopback(),
    tcp_link->is_active());

  if (this->links_.unbind(key, released_link) != 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: Unable to locate DataLink in order to "
               "release and it.\n"));

  } else if (release_pending) {
    released_link->set_release_pending (true);
    if (this->pending_release_links_.bind(key, released_link) != 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind released TcpDataLink to "
                 "pending_release_links_ map.\n"));
    }
  }

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *link;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::release_datalink_i() - ")
               ACE_TEXT("link with priority %d released.\n%C"),
               link->transport_priority(),
               buffer.str().c_str()));
  }
}

TcpInst*
TcpTransport::get_configuration()
{
  return this->tcp_config_.in();
}

/// This method is called by a TcpConnection object that has been
/// created and opened by our acceptor_ as a result of passively
/// accepting a connection on our local address.  The connection object
/// is "giving itself away" for us to manage.  Ultimately, the connection
/// object needs to be paired with a DataLink object that is (or will be)
/// expecting this passive connection to be established.
void
TcpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 TcpConnection* connection)
{
  DBG_ENTRY_LVL("TcpTransport", "passive_connection", 6);
  // Take ownership of the passed-in connection pointer.
  TcpConnection_rch connection_obj = connection;

  if (DCPS_debug_level > 9) {
    std::stringstream os;
    dump(os);

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::passive_connection() - ")
               ACE_TEXT("established with %C:%d.\n%C"),
               remote_address.get_host_name(),
               remote_address.get_port_number(),
               os.str().c_str()));
  }

  const PriorityKey key(connection->transport_priority(),
                        remote_address,
                        remote_address == this->tcp_config_->local_address_,
                        connection->is_connector());

  GuardType guard(this->connections_lock_);
  TcpDataLink_rch link;
  ConnectionEvent* evt;
  typedef std::multimap<ConnectionEvent*, PriorityKey>::iterator iter_t;
  for (iter_t iter = this->pending_connections_.begin();
       iter != pending_connections_.end(); ++iter) {
    if (iter->second == key) {
      GuardType guard(this->links_lock_);
      if (this->links_.find(key, link) == 0 /* found */) {
        evt = iter->first;
        //TODO: cleanup other multimap entries
        break;
      }
    }
  }
  if (!link.is_nil()) {
    if (this->connect_tcp_datalink(link.in(), connection) == -1) {
      //TODO: error
    }
    if (!evt->complete(link._retn())) {
      //TODO: other transport completed first
    }
    this->con_checker_->add(connection_obj);
    return;
  }

  VDBG_LVL((LM_DEBUG, "(%P|%t) # of bef connections: %d\n"
            , this->connections_.size()), 5);

  // Check and report and problems.
  ConnectionMap::iterator where = this->connections_.find(key);

  if (where != this->connections_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpTransport::passive_connection() - ")
               ACE_TEXT("connection with %C:%d at priority %d already exists, ")
               ACE_TEXT("overwriting previously established connection.\n"),
               remote_address.get_host_name(),
               remote_address.get_port_number(),
               connection->transport_priority()));
  }

  // Swap in the new connection.
  this->connections_[ key] = connection_obj;

  VDBG_LVL((LM_DEBUG, "(%P|%t) # of aftr connections: %d\n"
            , this->connections_.size()), 5);

  // Regardless of the outcome of the bind operation, let's tell any threads
  // that are wait()'ing on the connections_updated_ condition to check
  // the connections_ map again.

  this->connections_updated_.broadcast();

  // Enqueue the connection to the reconnect task that verifies if the connection
  // is re-established.
  this->con_checker_->add(connection_obj);
}

/// Actively establish a connection to the remote address.
int
TcpTransport::make_active_connection(const ACE_INET_Addr& remote_address,
                                     TcpDataLink* link)
{
  DBG_ENTRY_LVL("TcpTransport","make_active_connection",6);

  // Create the connection object here.
  TcpConnection_rch connection = new TcpConnection();

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *link;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::make_active connection() - ")
               ACE_TEXT("established with %C:%d and priority %d.\n%C"),
               remote_address.get_host_name(),
               remote_address.get_port_number(),
               link->transport_priority(),
               buffer.str().c_str()));
  }

  // Ask the connection object to attempt the active connection establishment.
  if (connection->active_connect(remote_address,
                                 this->tcp_config_->local_address_,
                                 link->transport_priority(),
                                 this->tcp_config_) != 0) {
    return -1;
  }

  return this->connect_tcp_datalink(link, connection.in());
}

int
TcpTransport::make_passive_connection
(const ACE_INET_Addr& remote_address,
 TcpDataLink*   link)
{
  DBG_ENTRY_LVL("TcpTransport","make_passive_connection",6);

  TcpConnection_rch connection;

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *link;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::make_passive_connection() - ")
               ACE_TEXT("waiting for connection from %C:%d and priority %d.\n%C"),
               remote_address.get_host_name(),
               remote_address.get_port_number(),
               link->transport_priority(),
               buffer.str().c_str()));
  }

  ACE_Time_Value abs_timeout(0);

  // TODO: Move this code somewhere?
  //if (this->tcp_config_->passive_connect_duration_ != 0) {
  //  abs_timeout.set(this->tcp_config_->passive_connect_duration_/1000,
  //                  this->tcp_config_->passive_connect_duration_%1000 * 1000);
  //  abs_timeout += ACE_OS::gettimeofday();
  //}

  PriorityKey key(link->transport_priority(), remote_address, link->is_loopback(), link->is_active());

  //VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   "
  //          "Passive connect timeout: %d milliseconds (0 == forever).\n",
  //          this->tcp_config_->passive_connect_duration_), 5);

  // Look in our connections_ map to see if the passive connection
  // has already been established for the remote_address.  If so, we
  // will extract it from the connections_ map and give it to the link.
  {
    GuardType guard(this->connections_lock_);

    while (true) {
      if ((abs_timeout != ACE_Time_Value::zero)
          && (abs_timeout <= ACE_OS::gettimeofday())) {
        // This doesn't necessarily represent an error.
        // It could just be a delay on the remote side. More a QOS issue.
        VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: Passive connection timedout.\n"), 5);
        return -1;
      }

      // check if there's already a connection waiting
      ConnectionMap::iterator position = this->connections_.find(key);

      if (position != this->connections_.end()) {
        connection = position->second;
        this->connections_.erase(position);
        break; // break out and continue with connection establishment
      }

      if (link->is_loopback()) {
        // The reservation lock needs be released at this point so the publisher
        // attached to same transport can make reservation and try to connect to
        // peer in.
        ACE_GUARD_RETURN (Reverse_Lock_t, unlock_guard, reverse_reservation_lock_, -1);
        // Now lets wait for an update
        wait_for_connection (abs_timeout);
      }
      else {
        //Now lets wait for an update
        wait_for_connection (abs_timeout);
      }
    }
  }

  // TBD SOON - Check to see if we we woke up because the Transport
  //            is shutting down.  If so, return a -1 now.

  return this->connect_tcp_datalink(link, connection.in());
}


void
TcpTransport::wait_for_connection(const ACE_Time_Value& abs_timeout)
{
  // Now lets wait for an update
  if (abs_timeout == ACE_Time_Value::zero) {
    this->connections_updated_.wait(0);

  } else {
    this->connections_updated_.wait(&abs_timeout);
  }
}


/// Common code used by make_active_connection() and make_passive_connection().
int
TcpTransport::connect_tcp_datalink(TcpDataLink* link,
                                   TcpConnection* connection)
{
  DBG_ENTRY_LVL("TcpTransport","connect_datalink",6);

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpTransport::connect_tcp_datalink() - ")
               ACE_TEXT("creating send strategy with priority %d.\n"),
               link->transport_priority()));
  }

  TransportSendStrategy_rch send_strategy
  = new TcpSendStrategy(
    link,
    this->tcp_config_.in(),
    connection,
    new TcpSynchResource(
      connection,
      this->tcp_config_->max_output_pause_period_),
    this->reactor_task_.in(),
    link->transport_priority());

  TransportReceiveStrategy_rch receive_strategy =
    new TcpReceiveStrategy(link,
                                 connection,
                                 this->reactor_task_.in());

  if (link->connect(connection,
                    send_strategy.in(),
                    receive_strategy.in()) != 0) {
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

  PriorityKey key(connection->transport_priority(),
                  connection->get_remote_address(),
                  connection->get_remote_address() == this->tcp_config_->local_address_,
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
      return link->reconnect(connection.in());
    }
  }

  return 0;
}

}
}
