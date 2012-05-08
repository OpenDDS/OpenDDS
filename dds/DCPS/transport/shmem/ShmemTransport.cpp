/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemTransport.h"
#include "ShmemInst.h"
#include "ShmemSendStrategy.h"
#include "ShmemReceiveStrategy.h"

#include "dds/DCPS/AssociationData.h"

#include "ace/Log_Msg.h"

namespace OpenDDS {
namespace DCPS {

ShmemTransport::ShmemTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

ShmemDataLink*
ShmemTransport::make_datalink(const ACE_TString& remote_address, bool active)
{
  ShmemDataLink_rch link;
  ACE_NEW_RETURN(link, ShmemDataLink(this), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  link->configure(this->config_i_.in());

  // Assign send strategy:
  ShmemSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, ShmemSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  ShmemReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, ShmemReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Open logical connection:
  if (!link->open(remote_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

DataLink*
ShmemTransport::find_datalink_i(const RepoId& /*local_id*/,
                                const RepoId& /*remote_id*/,
                                const TransportBLOB& remote_data,
                                bool /*remote_reliable*/,
                                const ConnectionAttribs& attribs,
                                bool active)
{
  //TODO
  return 0;
}

DataLink*
ShmemTransport::connect_datalink_i(const RepoId& /*local_id*/,
                                 const RepoId& /*remote_id*/,
                                 const TransportBLOB& remote_data,
                                 bool /*remote_reliable*/,
                                 const ConnectionAttribs& attribs)
{
  //TODO
  return 0;
}

DataLink*
ShmemTransport::accept_datalink(ConnectionEvent& ce)
{
  const std::string ttype = "shmem";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();

  // std::vector<PriorityKey> keys;
  GuardType guard(this->connections_lock_);

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {
      //const PriorityKey key =
      //  this->blob_to_key(ce.remote_association_.remote_data_[idx].data,
      //                    ce.attribs_.priority_, false /*active == false*/);

      //keys.push_back(key);
    }
  }

  //for (size_t i = 0; i < keys.size(); ++i) {
  //  if (this->pending_server_link_keys_.find(keys[i]) !=
  //      this->pending_server_link_keys_.end()) {
  //    // Handshake already seen, add to server_link_keys_ and
  //    // return server_link_
  //    this->pending_server_link_keys_.erase(keys[i]);
  //    this->server_link_keys_.insert(keys[i]);
  //    VDBG((LM_DEBUG, "(%P|%t) ShmemTransport::accept_datalink completing\n"));
  //    return ShmemDataLink_rch(this->server_link_)._retn();
  //  } else {
  //    // Add to pending and wait for handshake
  //    this->pending_connections_.insert(
  //      std::pair<ConnectionEvent* const, PriorityKey>(&ce, keys[i]));
  //    VDBG((LM_DEBUG, "(%P|%t) ShmemTransport::accept_datalink pending\n"));
  //  }
  //}

  // Let TransportClient::associate() wait for the handshake
  return 0;
}

void
ShmemTransport::stop_accepting(ConnectionEvent& ce)
{
  GuardType guard(this->connections_lock_);
  typedef std::multimap<ConnectionEvent*, ACE_TString>::iterator iter_t;
  std::pair<iter_t, iter_t> range = this->pending_connections_.equal_range(&ce);
  this->pending_connections_.erase(range.first, range.second);
  VDBG((LM_DEBUG, "(%P|%t) ShmemTransport::stop_accepting\n"));
}

bool
ShmemTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<ShmemInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ShmemTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  this->config_i_->_add_ref();

  //TODO: create semaphore-task and other per-transport-instance objects

  return true;
}

void
ShmemTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  GuardType guard(this->links_lock_);
  for (ShmemDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  this->links_.clear();

  this->config_i_ = 0;
}

bool
ShmemTransport::connection_info_i(TransportLocator& info) const
{
  info.transport_type = "shmem";
  //TODO
  return true;
}
//
//ACE_INET_Addr
//ShmemTransport::get_connection_addr(const TransportBLOB& data) const
//{
//  return local_address;
//}

void
ShmemTransport::release_datalink(DataLink* link)
{
  GuardType guard(this->links_lock_);
  for (ShmemDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      link->stop();
      this->links_.erase(it);
      return;
    }
  }
}

void
ShmemTransport::passive_connection(const ACE_TString& remote_address,
                                   ACE_Message_Block* data)
{
  // Use the addr to find this connection in pending connections_ and
  // to locate the ConnectionEvent
  ConnectionEvent* evt = 0;
  {
    GuardType guard(this->connections_lock_);
    typedef std::multimap<ConnectionEvent*, ACE_TString>::iterator iter_t;
    for (iter_t iter = this->pending_connections_.begin();
         iter != pending_connections_.end(); ++iter) {
      if (iter->second == remote_address) {
        evt = iter->first;
        break;
      }
    }

    // Send an ack so that the active side can return from
    // connect_datalink_i().
    //TODO

    if (evt != 0) { // found in pending_connections_
      // remove other entries for this ConnectionEvent in pending_connections_
      std::pair<iter_t, iter_t> range =
        this->pending_connections_.equal_range(evt);
      this->pending_connections_.erase(range.first, range.second);

      VDBG((LM_DEBUG, "(%P|%t) ShmemTransport::passive_connection completing\n"));
      // Signal TransportClient::associate() via the ConnectionEvent
      // to let it know that we found a good connection.
//      evt->complete(static_rchandle_cast<DataLink>(this->server_link_));

      // Add an entry to server_link_keys_ so we can find the
      // "connection" for that key.
//      this->server_link_keys_.insert(key);
    } else {

      // Add an entry to pending_server_link_keys_ so we can finish
      // associating in accept_datalink().
//      this->pending_server_link_keys_.insert(key);

      VDBG((LM_DEBUG, "(%P|%t) ShmemTransport::passive_connection pending\n"));
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
