/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REPOIDSETMAP_H
#define OPENDDS_DCPS_REPOIDSETMAP_H

#include "dds/DCPS/dcps_export.h"
#include "RepoIdSet.h"
#include "TransportDefs.h"
#include "RepoIdSet_rch.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/Serializer.h"
#include "ace/Synch.h"
#include <map>

namespace OpenDDS {
namespace DCPS {

class RepoIdSetMap;

class OpenDDS_Dcps_Export RepoIdSetMap {
public:

  typedef std::map<RepoId, RepoIdSet_rch, GUID_tKeyLessThan> MapType;

  RepoIdSetMap();
  virtual ~RepoIdSetMap();

  int        insert(RepoId key, RepoId value);
  RepoIdSet* find(RepoId key);

  int        remove(RepoId key, RepoId value);
  RepoIdSet* remove_set(RepoId key);

  int release_publisher(RepoId subscriber_id, RepoId publisher_id);

  size_t size() const;

  /// Give access to the underlying map for iteration purposes.
  MapType& map();
  const MapType& map() const;

  /// The size of the serialized map.
  size_t marshaled_size();

  /// Serialize this map. The data in the stream:
  /// size of this map, list of key(repoid)-value(RepoIdSet).
  ACE_Message_Block* marshal(bool byte_order);

  /// Demarshal the serialized data of a RepoIdSetMap.
  int demarshal(ACE_Message_Block* acks, bool byte_order);

  /// List the key of this map.
  void get_keys(RepoIdSet& keys);

  void operator= (const RepoIdSetMap &);

  void clear();

  void dump();

private:

  RepoIdSet* find_or_create(RepoId key);

  MapType  map_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "RepoIdSetMap.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_REPOIDSETMAP_H */
