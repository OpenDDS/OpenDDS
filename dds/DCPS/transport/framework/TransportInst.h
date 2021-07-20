/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTINST_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTINST_H

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/ReactorTask_rch.h"
#include "dds/DCPS/TimeDuration.h"

#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "TransportImpl.h"
#include "ace/Synch_Traits.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

/**
 * @class TransportInst
 *
 * @brief Base class to hold configuration settings for TransportImpls.
 *
 * Each transport implementation will need to define a concrete
 * subclass of the TransportInst class.  The base
 * class (TransportInst) contains configuration settings that
 * are common to all (or most) concrete transport implementations.
 * The concrete transport implementation defines any configuration
 * settings that it requires within its concrete subclass of this
 * TransportInst base class.
 *
 * The TransportInst object is supplied to the
 * TransportImpl::configure() method.
 */
class OpenDDS_Dcps_Export TransportInst : public RcObject {
public:

  const OPENDDS_STRING& name() const { return name_; }

  /// Overwrite the default configurations with the configuration from the
  /// given section in the ACE_Configuration_Heap object.
  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  void dump() const;
  virtual OPENDDS_STRING dump_to_str() const;

  /// Format name of transport configuration parameter for use in
  /// conjunction with dump(std::ostream& os).
  static OPENDDS_STRING formatNameForDump(const char* name);

  const OPENDDS_STRING transport_type_;

  /// Number of pre-created link (list) objects per pool for the
  /// "send queue" of each DataLink.
  size_t queue_messages_per_pool_;

  /// Initial number of pre-allocated pools of link (list) objects
  /// for the "send queue" of each DataLink.
  size_t queue_initial_pools_;

  /// Max size (in bytes) of a packet (packet header + sample(s))
  ACE_UINT32 max_packet_size_;

  /// Max number of samples that should ever be in a single packet.
  size_t max_samples_per_packet_;

  /// Optimum size (in bytes) of a packet (packet header + sample(s)).
  ACE_UINT32 optimum_packet_size_;

  /// Flag for whether a new thread is needed for connection to
  /// send without backpressure.
  bool thread_per_connection_;

  /// Delay in milliseconds that the datalink should be released after all
  /// associations are removed. The default value is 10 seconds.
  long datalink_release_delay_;

  /// The number of chunks used to size allocators for transport control
  /// samples. The default value is 32.
  size_t datalink_control_chunks_;

  /// Maximum time to store incoming fragments of incomplete data samples.
  /// The expiration time is relative to the last received fragment.
  TimeDuration fragment_reassembly_timeout_;

  /// Does the transport as configured support RELIABLE_RELIABILITY_QOS?
  virtual bool is_reliable() const = 0;

  /// Does the transport require a CDR-encapsulated data payload?
  virtual bool requires_cdr_encapsulation() const { return false; }

  /// Populate a transport locator sequence.  Return the number of "locators."
  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const = 0;

  ICE::Endpoint* get_ice_endpoint();
  void rtps_relay_only_now(bool flag);
  void use_rtps_relay_now(bool flag);
  void use_ice_now(bool flag);

  virtual void update_locators(const RepoId& /*remote_id*/,
                               const TransportLocatorSeq& /*locators*/) {}

  ReactorTask_rch reactor_task();

protected:

  TransportInst(const char* type,
                const OPENDDS_STRING& name);

  virtual ~TransportInst();

  void set_port_in_addr_string(OPENDDS_STRING& addr_str, u_short port_number);

  mutable ACE_SYNCH_MUTEX lock_;
  bool shutting_down_;

private:

  /// Adjust the configuration values which gives warning on adjusted
  /// value.
  void adjust_config_value();

  friend class TransportRegistry;
  void shutdown();

  friend class TransportClient;
 protected:
  TransportImpl_rch impl();
 private:
  virtual TransportImpl_rch new_impl() = 0;

  const OPENDDS_STRING name_;

  TransportImpl_rch impl_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "TransportInst.inl"
#endif /* __ACE_INLINE__ */

#endif
