/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMTRANSPORT_H
#define OPENDDS_SHMEMTRANSPORT_H

#include "Shmem_Export.h"

#include "ShmemDataLink_rch.h"
#include "ShmemDataLink.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"

#include "dds/DCPS/PoolAllocator.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ShmemInst;

class OpenDDS_Shmem_Export ShmemTransport : public TransportImpl {
public:
  explicit ShmemTransport(ShmemInst& inst);

  // used by our DataLink:
  ShmemAllocator* alloc() { return alloc_.get(); }
  std::string address();
  void signal_semaphore();

  ShmemInst& config() const;

protected:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               const TransportClient_rch& client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              const TransportClient_rch& client);

  virtual void stop_accepting_or_connecting(const TransportClient_wrch& client,
                                            const RepoId& remote_id);

  bool configure_i(ShmemInst& config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "shmem"; }

private:

  /// Create a new link (using make_datalink) and add it to the map
  DataLink_rch add_datalink(const std::string& remote_address);

  /// Create the DataLink object and start it
  ShmemDataLink_rch make_datalink(const std::string& remote_address);

  std::pair<std::string, std::string> blob_to_key(const TransportBLOB& blob);

  void read_from_links(); // callback from ReadTask

  typedef ACE_Thread_Mutex        LockType;
  typedef ACE_Guard<LockType>     GuardType;

  LockType links_lock_;

  /// Map of fully associated DataLinks for this transport.  Protected
  /// by links_lock_.
  typedef OPENDDS_MAP(std::string, ShmemDataLink_rch) ShmemDataLinkMap;
  ShmemDataLinkMap links_;

  unique_ptr<ShmemAllocator> alloc_;

  class ReadTask : public ACE_Task_Base {
  public:
    ReadTask(ShmemTransport* outer, ACE_sema_t semaphore);
    int svc();
    void stop();
    void signal_semaphore();

  private:
    ShmemTransport* outer_;
    ACE_sema_t semaphore_;
    bool stopped_;
  };
  unique_ptr<ReadTask> read_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_SHMEMTRANSPORT_H */
