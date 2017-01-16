/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCONFIG_H
#define OPENDDS_DCPS_TRANSPORTCONFIG_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/PoolAllocator.h"
#include "TransportInst.h"
#include "TransportInst_rch.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportConfig : public RcObject<ACE_SYNCH_MUTEX> {
public:

  static const unsigned long DEFAULT_PASSIVE_CONNECT_DURATION = 60000;

  OPENDDS_STRING name() const { return name_; }

  typedef OPENDDS_VECTOR(TransportInst_rch) InstancesType;
  InstancesType instances_;

  bool swap_bytes_;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection.
  /// The default is 60 seconds
  unsigned long passive_connect_duration_;

  /// Insert the TransportInst in sorted order (by name) in the instances_ list.
  /// Use when the names of the TransportInst objects are specifically assigned
  /// to have the sorted order make sense.
  void sorted_insert(const TransportInst_rch& inst);

  void populate_locators(OpenDDS::DCPS::TransportLocatorSeq& trans_info) const;

private:
  friend class TransportRegistry;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit TransportConfig(const OPENDDS_STRING& name);
  ~TransportConfig();

  const OPENDDS_STRING name_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_H */
