/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RECEIVELISTENERSET_H
#define OPENDDS_DCPS_RECEIVELISTENERSET_H

#include "dds/DCPS/RcObject_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/PoolAllocator.h"
#include "ace/Synch_Traits.h"
#include "ace/Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportReceiveListener;
class ReceivedDataSample;
typedef RcHandle<TransportReceiveListener> TransportReceiveListener_rch;

class OpenDDS_Dcps_Export ReceiveListenerSet :
      public RcObject<ACE_SYNCH_MUTEX> {
public:

  enum ConstrainReceiveSet {
    SET_EXCLUDED,
    SET_INCLUDED
  };

  typedef OPENDDS_MAP_CMP(RepoId, TransportReceiveListener_rch, GUID_tKeyLessThan) MapType;

  ReceiveListenerSet();
  ReceiveListenerSet(const ReceiveListenerSet&);
  ReceiveListenerSet& operator=(const ReceiveListenerSet&);
  virtual ~ReceiveListenerSet();

  int insert(RepoId                              subscriber_id,
             const TransportReceiveListener_rch& listener);
  int remove(RepoId subscriber_id);
  void remove_all(const GUIDSeq& to_remove);

  ssize_t size() const;

  void data_received(const ReceivedDataSample& sample,
                     const RepoIdSet& incl_excl,
                     ConstrainReceiveSet constrain);
  void data_received(const ReceivedDataSample& sample, const RepoId& readerId);

  /// Give access to the underlying map for iteration purposes.
  MapType& map();
  const MapType& map() const;

  /// Check if the key is in the map and if it's the only left entry
  /// in the map.
  bool exist(const RepoId& key, bool& last);
  bool exist(const RepoId& local_id);

  void get_keys(ReaderIdSeq & ids);

  void clear();

private:

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  /// This lock will protect the map.
  mutable LockType lock_;

  MapType  map_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_RECEIVELISTENERSET_H */
