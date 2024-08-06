/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCONFIG_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCONFIG_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/PoolAllocator.h"
#include "TransportInst.h"
#include "TransportInst_rch.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportConfig : public virtual RcObject {
public:

  static const unsigned long DEFAULT_PASSIVE_CONNECT_DURATION = 60000;

  const String& name() const { return name_; }
  const String& config_prefix() const { return config_prefix_; }
  String config_key(const String& key) const
  {
    return ConfigPair::canonicalize(config_prefix_ + "_" + key);
  }

  typedef OPENDDS_VECTOR(TransportInst_rch) InstancesType;
  InstancesType instances_;

  ConfigValue<TransportConfig, bool> swap_bytes_;
  void swap_bytes(bool flag);
  bool swap_bytes() const;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection.
  /// The default is 60 seconds
  ConfigValueRef<TransportConfig, TimeDuration> passive_connect_duration_;
  void passive_connect_duration(const TimeDuration& pcd);
  TimeDuration passive_connect_duration() const;

  ConfigValueRef<TransportConfig, ConfigStoreImpl::StringList> transports_;
  void transports(const ConfigStoreImpl::StringList& t);
  ConfigStoreImpl::StringList transports() const;

  /// Insert the TransportInst in sorted order (by name) in the instances_ list.
  /// Use when the names of the TransportInst objects are specifically assigned
  /// to have the sorted order make sense.
  void sorted_insert(const TransportInst_rch& inst);

  void populate_locators(OpenDDS::DCPS::TransportLocatorSeq& trans_info,
                         DDS::DomainId_t domain) const;

  bool uses_template() const;

private:
  friend class TransportRegistry;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit TransportConfig(const String& name);
  ~TransportConfig();

  const String name_;
  const String config_prefix_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_H */
