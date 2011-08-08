/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_CLIENT_H
#define OPENDDS_DCPS_TRANSPORT_CLIENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportImpl.h"
#include "DataLinkSet.h"

#include "ace/Time_Value.h"

#include <vector>

// Forward definition of a test-friendly class in the global name space
class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class EntityImpl;
class TransportInst;
class AssocationInfo;
class ReaderIdSeq;
class WriterIdSeq;
class DataSampleList;

/**
 * @brief Mix-in class for DDS entities which directly use the transport layer.
 *
 * DataReaderImpl and DataWriterImpl are TransportClients.  The TransportClient
 * class manages the TransportImpl objects that represent the available
 * communication mechanisms and the DataLink objects that represent the
 * currently active communication channels to peers.
 */
class OpenDDS_Dcps_Export TransportClient {
protected:
  TransportClient();
  virtual ~TransportClient();


  // Local setup:

  void enable_transport();
  bool swap_bytes() const { return swap_bytes_; }
  const TransportLocatorSeq& connection_info() const { return conn_info_; }


  // Managing associations to remote peers:

  bool associate(const AssociationData& peer, bool active);
  void disassociate(const RepoId& peerId);


  // Data transfer:

  bool send_response(const RepoId& peer, ACE_Message_Block* payload); // [DR]
  void send(const DataSampleList& samples);
  SendControlStatus send_control(ACE_Message_Block* msg, void* extra = 0);
  bool remove_sample(const DataSampleListElement* sample);
  bool remove_all_msgs();

private:

  // Implemented by derived classes (DataReaderImpl/DataWriterImpl)
  virtual bool check_transport_qos(const TransportInst& inst) = 0;
  virtual const RepoId& get_repo_id() const = 0;
  virtual CORBA::Long get_priority_value(const AssociationData& data) const = 0;

  // transport_detached() is called from TransportImpl when it shuts down
  friend class TransportImpl;
  void transport_detached(TransportImpl* which);

  // helpers
  void add_link(const DataLink_rch& link, const RepoId& peer);
  TransportSendListener* get_send_listener();
  TransportReceiveListener* get_receive_listener();

  // A class, normally provided by an unit test, who needs access to a client's
  // privates.
  friend class ::DDS_TEST;

  typedef std::map<RepoId, DataLink_rch, GUID_tKeyLessThan> DataLinkIndex;


  // Associated Impls and DataLinks:

  std::vector<TransportImpl_rch> impls_;
  DataLinkSet links_;
  DataLinkIndex data_link_index_;


  // Configuration details:

  bool swap_bytes_;
  ACE_Time_Value passive_connect_duration_;


  TransportLocatorSeq conn_info_;
  ACE_Thread_Mutex lock_;
  RepoId repo_id_;


  class MultiReservLock {
  public:
    explicit MultiReservLock(const std::vector<TransportImpl_rch>& impls);
    int acquire();
    int tryacquire();
    int release();
    int remove();
  private:
    typedef int (TransportImpl::ReservationLockType::*PMF)();
    int action_fwd(PMF function, PMF undo);
    int action_rev(PMF function);
    const std::vector<TransportImpl_rch>& impls_;
    std::set<TransportImpl*> sorted_;
  };
};

}
}

#endif
