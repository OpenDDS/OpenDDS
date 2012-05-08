/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMTRANSPORT_H
#define OPENDDS_SHMEMTRANSPORT_H

#include "Shmem_Export.h"

#include "ShmemDataLink.h"
#include "ShmemDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

class ShmemInst;

class OpenDDS_Shmem_Export ShmemTransport : public TransportImpl {
public:
  explicit ShmemTransport(const TransportInst_rch& inst);

  void passive_connection(const ACE_TString& remote_address,
                          ACE_Message_Block* data);

protected:
  virtual DataLink* find_datalink_i(const RepoId& local_id,
                                    const RepoId& remote_id,
                                    const TransportBLOB& remote_data,
                                    bool remote_reliable,
                                    const ConnectionAttribs& attribs,
                                    bool active);

  virtual DataLink* connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& remote_data,
                                       bool remote_reliable,
                                       const ConnectionAttribs& attribs);

  virtual DataLink* accept_datalink(ConnectionEvent& ce);
  virtual void stop_accepting(ConnectionEvent& ce);

  virtual bool configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "shmem"; }

private:
  ShmemDataLink* make_datalink(const ACE_TString& remote_address, bool active);

  RcHandle<ShmemInst> config_i_;

  typedef ACE_Thread_Mutex        LockType;
  typedef ACE_Guard<LockType>     GuardType;

  /// This lock is used to protect the client_links_ data member.
  LockType links_lock_;

  /// Map of fully associated DataLinks for this transport.  Protected
  // by links_lock_.
  typedef std::map<ACE_TString, ShmemDataLink_rch> ShmemDataLinkMap;
  ShmemDataLinkMap links_;

  /// This protects the pending_connections_ data member.
  LockType connections_lock_;

  /// Locked by connections_lock_.  Tracks expected connections
  /// that we have learned about in accept_datalink() but have
  /// not yet performed the handshake.
  std::multimap<ConnectionEvent*, ACE_TString> pending_connections_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_SHMEMTRANSPORT_H */
