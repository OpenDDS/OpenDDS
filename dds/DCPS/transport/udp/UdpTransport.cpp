/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpTransport.h"
#include "UdpInst.h"
#include "UdpSendStrategy.h"
#include "UdpReceiveStrategy.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/AssociationData.h"

namespace OpenDDS {
namespace DCPS {

UdpTransport::UdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

UdpDataLink*
UdpTransport::make_datalink(const ACE_INET_Addr& remote_address, bool active)
{
  UdpDataLink_rch link;
  ACE_NEW_RETURN(link, UdpDataLink(this, active), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send strategy:
  UdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, UdpSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  UdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, UdpReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Open logical connection:
  if (!link->open(remote_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

DataLink*
UdpTransport::find_datalink_i(const RepoId& /*local_id*/,
                              const RepoId& /*remote_id*/,
                              const TransportBLOB& remote_data,
                              CORBA::Long priority,
                              bool active)
{
  ACE_INET_Addr remote_address = get_connection_addr(remote_data);

  bool is_loopback = remote_address == this->config_i_->local_address_;
  PriorityKey key(priority, remote_address, is_loopback, active);

  if (active) {
    GuardType guard(this->client_links_lock_);
    UdpDataLinkMap::iterator it(this->client_links_.find(key));
    if (it != this->client_links_.end()) {
      return UdpDataLink_rch(it->second)._retn(); // found
    }
  } else {
    GuardType guard(this->connections_lock_);
    if (this->server_link_keys_.find(key) == this->server_link_keys_.end()) {
      return 0;
    } else {
      // return a reference to the one and only server data link
      return UdpDataLink_rch(this->server_link_)._retn();
    }
  }

  return 0;
}

DataLink*
UdpTransport::connect_datalink_i(const RepoId& /*local_id*/,
                                 const RepoId& /*remote_id*/,
                                 const TransportBLOB& remote_data,
                                 CORBA::Long priority)
{
  ACE_INET_Addr remote_address = get_connection_addr(remote_data);
  const bool active = true;

  PriorityKey key = this->blob_to_key(remote_data, priority, active);

  // Create new DataLink for logical connection:
  UdpDataLink_rch link = make_datalink(remote_address, active);
  GuardType guard(this->client_links_lock_);
  this->client_links_.insert(UdpDataLinkMap::value_type(key, link));

  return link._retn();
}

DataLink*
UdpTransport::accept_datalink(ConnectionEvent& ce)
{
  const std::string ttype = "udp";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();

  std::vector<PriorityKey> keys;
  GuardType guard(this->connections_lock_);

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {
      const PriorityKey key =
        this->blob_to_key(ce.remote_association_.remote_data_[idx].data,
                          ce.priority_, false /*active == false*/);

      keys.push_back(key);
    }
  }

  for (size_t i = 0; i < keys.size(); ++i) {
    if (this->pending_server_link_keys_.find(keys[i]) !=
        this->pending_server_link_keys_.end()) {
      // Handshake already seen, add to server_link_keys_ and
      // return server_link_
      this->pending_server_link_keys_.erase(keys[i]);
      this->server_link_keys_.insert(keys[i]);
      return UdpDataLink_rch(this->server_link_)._retn();
    } else {
      // Add to pending and wait for handshake
      this->pending_connections_.insert(std::make_pair(&ce, keys[i]));
    }
  }

  // Let TransportClient::associate() wait for the handshake
  return 0;
}

void
UdpTransport::stop_accepting(ConnectionEvent& ce)
{
  GuardType guard(this->connections_lock_);
  typedef std::multimap<ConnectionEvent*, PriorityKey>::iterator iter_t;
  std::pair<iter_t, iter_t> range = this->pending_connections_.equal_range(&ce);
  this->pending_connections_.erase(range.first, range.second);
}

bool
UdpTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<UdpInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  // Our "server side" data link is created here, similar to the acceptor_
  // in the TcpTransport implementation.  This establishes a socket as an
  // endpoint that we can advertise to peers via connection_info_i().

  this->server_link_ = make_datalink(ACE_INET_Addr(), false);

  return true;
}

void
UdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  GuardType guard(this->client_links_lock_);
  for (UdpDataLinkMap::iterator it(this->client_links_.begin());
       it != this->client_links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  this->client_links_.clear();

  this->config_i_ = 0;
}

bool
UdpTransport::connection_info_i(TransportLocator& info) const
{
  NetworkAddress network_address(this->config_i_->local_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  info.transport_type = "udp";
  info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));

  return true;
}

ACE_INET_Addr
UdpTransport::get_connection_addr(const TransportBLOB& data) const
{
  ACE_INET_Addr local_address;
  NetworkAddress network_address;

  size_t len = data.length();
  const char* buffer = reinterpret_cast<const char*>(data.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;

  network_address.to_addr(local_address);

  return local_address;
}

void
UdpTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  GuardType guard(this->client_links_lock_);
  for (UdpDataLinkMap::iterator it(this->client_links_.begin());
       it != this->client_links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      this->client_links_.erase(it);
      return;
    }
  }
}

PriorityKey
UdpTransport::blob_to_key(const TransportBLOB& remote,
                          CORBA::Long priority,
                          bool active)
{
  NetworkAddress network_order_address;
  ACE_InputCDR cdr((const char*)remote.get_buffer(), remote.length());

  if ((cdr >> network_order_address) == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: UdpTransport::blob_to_key")
               ACE_TEXT(" failed to de-serialize the NetworkAddress\n")));
  }

  ACE_INET_Addr remote_address;
  network_order_address.to_addr(remote_address);
  const bool is_loopback = remote_address == this->config_i_->local_address_;

  return PriorityKey(priority, remote_address, is_loopback, active);
}

void
UdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 ACE_Message_Block* data)
{
  size_t octet_size = data->length() - sizeof(CORBA::Long);
  CORBA::Long priority;
  Serializer serializer(data);
  serializer >> priority;
  TransportBLOB blob(octet_size);
  blob.length(octet_size);
  serializer.read_octet_array(blob.get_buffer(), octet_size);

  PriorityKey key = this->blob_to_key(blob, priority, false /* passive */);

  // Use the key to find this connection in pending connections_ and
  // to locate the ConnectionEvent
  ConnectionEvent* evt = 0;
  {
    GuardType guard(this->connections_lock_);
    typedef std::multimap<ConnectionEvent*, PriorityKey>::iterator iter_t;
    for (iter_t iter = this->pending_connections_.begin();
         iter != pending_connections_.end(); ++iter) {
      if (iter->second == key) {
        evt = iter->first;
        break;
      }
    }

    // Send an ack so that the active side can return from
    // connect_datalink_i().  This is just a single byte of
    // arbitrary data, the remote side is not yet using the
    // framework (TransportHeader, DataSampleHeader,
    // ReceiveStrategy).
    const char ack_data = 23;
    this->server_link_->socket().send(&ack_data, 1, remote_address);

    if (evt != 0) { // found in pending_connections_
      // remove other entries for this ConnectionEvent in pending_connections_
      std::pair<iter_t, iter_t> range =
        this->pending_connections_.equal_range(evt);
      this->pending_connections_.erase(range.first, range.second);

      // Signal TransportClient::associate() via the ConnectionEvent
      // to let it know that we found a good connection.
      evt->complete(this->server_link_.in());

      // Add an entry to server_link_keys_ so we can find the
      // "connection" for that key.
      this->server_link_keys_.insert(key);
    } else {

      // Add an entry to pending_server_link_keys_ so we can finish
      // associating in accept_datalink().
      this->pending_server_link_keys_.insert(key);
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
