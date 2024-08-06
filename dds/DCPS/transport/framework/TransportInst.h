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

#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "TransportImpl.h"

#include <dds/DCPS/ConfigStoreImpl.h>
#include <dds/DCPS/EventDispatcher.h>
#include <dds/DCPS/NetworkAddress.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/ReactorTask_rch.h>
#include <dds/DCPS/TimeDuration.h>
#include <dds/DCPS/dcps_export.h>

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/OpenddsDcpsExtC.h>

#include <ace/Synch_Traits.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

class DomainParticipantImpl;

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
class OpenDDS_Dcps_Export TransportInst : public virtual RcObject {
public:

  static const long DEFAULT_DATALINK_RELEASE_DELAY = 10000;
  static const size_t DEFAULT_DATALINK_CONTROL_CHUNKS = 32u;

  const String& name() const { return name_; }
  const String& config_prefix() const { return config_prefix_; }
  String config_key(const String& key) const
  {
    return ConfigPair::canonicalize(config_prefix_ + "_" + key);
  }

  bool is_template() const { return is_template_; }

  /// Diagnostic aid.
  void dump(DDS::DomainId_t domain) const;
  virtual OPENDDS_STRING dump_to_str(DDS::DomainId_t domain) const;

  /// Format name of transport configuration parameter for use in
  /// conjunction with dump(std::ostream& os).
  static OPENDDS_STRING formatNameForDump(const char* name);

  const OPENDDS_STRING transport_type_;

  /// Max size (in bytes) of a packet (packet header + sample(s))
  void max_packet_size(ACE_UINT32 mps);
  ACE_UINT32 max_packet_size() const;

  /// Max number of samples that should ever be in a single packet.
  void max_samples_per_packet(size_t mspp);
  size_t max_samples_per_packet() const;

  /// Optimum size (in bytes) of a packet (packet header + sample(s)).
  void optimum_packet_size(ACE_UINT32 ops);
  ACE_UINT32 optimum_packet_size() const;

  /// Flag for whether a new thread is needed for connection to
  /// send without backpressure.
  void thread_per_connection(bool tpc);
  bool thread_per_connection() const;

  /// Delay in milliseconds that the datalink should be released after all
  /// associations are removed. The default value is 10 seconds.
  void datalink_release_delay(long drd);
  long datalink_release_delay() const;

  /// The number of chunks used to size allocators for transport control
  /// samples. The default value is 32.
  void datalink_control_chunks(size_t dcc);
  size_t datalink_control_chunks() const;

  /// Maximum time to store incoming fragments of incomplete data samples.
  /// The expiration time is relative to the last received fragment.
  void fragment_reassembly_timeout(const TimeDuration& frt);
  TimeDuration fragment_reassembly_timeout() const;

  /// Preallocated chunks in allocator for message blocks.
  /// Default (0) is to use built-in constants in TransportReceiveStrategy
  void receive_preallocated_message_blocks(size_t rpmb);
  size_t receive_preallocated_message_blocks() const;

  /// Preallocated chunks in allocator for data blocks and data buffers.
  /// Default (0) is to use built-in constants in TransportReceiveStrategy
  void receive_preallocated_data_blocks(size_t rpdb);
  size_t receive_preallocated_data_blocks() const;

  /// Does the transport as configured support RELIABLE_RELIABILITY_QOS?
  virtual bool is_reliable() const = 0;

  /// Does the transport require a CDR-encapsulated data payload?
  virtual bool requires_cdr_encapsulation() const { return false; }

  /// Populate a transport locator sequence.  Return the number of "locators."
  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info,
                                  ConnectionInfoFlags flags,
                                  DDS::DomainId_t domain) const = 0;

  DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint(DDS::DomainId_t domain,
                                                     DomainParticipantImpl* participant);
  void rtps_relay_only_now(bool flag);
  void use_rtps_relay_now(bool flag);
  void use_ice_now(bool flag);

  virtual void update_locators(const GUID_t& /*remote_id*/,
                               const TransportLocatorSeq& /*locators*/,
                               DDS::DomainId_t /*domain*/,
                               DomainParticipantImpl* /*participant*/) {}

  virtual void get_last_recv_locator(const GUID_t& /*remote_id*/,
                                     const GuidVendorId_t& /*vendor_id*/,
                                     TransportLocator& /*locators*/,
                                     DDS::DomainId_t /*domain*/,
                                     DomainParticipantImpl* /*participant*/) {}

  ReactorTask_rch reactor_task(DDS::DomainId_t domain,
                               DomainParticipantImpl* participant);
  EventDispatcher_rch event_dispatcher(DDS::DomainId_t domain,
                                       DomainParticipantImpl* participant);

  /**
   * @{
   * The RtpsUdpSendStrategy can be configured to simulate a lossy
   * connection by probabilistically not sending RTPS messages.  This
   * capability is enabled by setting the flag to true.
   *
   * The coefficients m and b correspond to a linear model whose
   * argument is the length of the RTPS message.  The probablility of
   * dropping the message is p = m * message_length + b.  When sending
   * a message, a random number is selected from [0.0, 1.0].  If the
   * random number is less than the drop probability, the message is
   * dropped.
   *
   * The flag and coefficient can be changed dynamically.
   *
   * Examples
   *
   * m = 0 and b = .5 - the probability of dropping a message is .5
   * regardless of message length.
   *
   * m = .001 and b = .2 - the probability of dropping a 200-byte
   * message is .4.  In this example, all messages longer than 800
   * bytes will be dropped.
   */
  void drop_messages(bool flag);
  bool drop_messages() const;

  void drop_messages_m(double m);
  double drop_messages_m() const;

  void drop_messages_b(double b);
  double drop_messages_b() const;
  /**@}*/

  void count_messages(bool flag);
  bool count_messages() const;

  virtual void append_transport_statistics(TransportStatisticsSequence& /*seq*/,
                                           DDS::DomainId_t /*domain*/,
                                           DomainParticipantImpl* /*participant*/) {}

  static void set_port_in_addr_string(OPENDDS_STRING& addr_str, u_short port_number);

  void remove_participant(DDS::DomainId_t domain,
                          DomainParticipantImpl* participant);

protected:

  TransportInst(const char* type,
                const OPENDDS_STRING& name,
                bool is_template = false);

  virtual ~TransportInst();

  mutable ACE_SYNCH_MUTEX lock_;
  bool shutting_down_;

private:

  friend class TransportRegistry;
  void shutdown();

  friend class TransportClient;
 protected:
  TransportImpl_rch get_or_create_impl(DDS::DomainId_t domain,
                                       DomainParticipantImpl* participant);
  TransportImpl_rch get_impl(DDS::DomainId_t domain,
                             DomainParticipantImpl* participant);
 private:
  virtual TransportImpl_rch new_impl(DDS::DomainId_t domain) = 0;

  String instantiation_rule() const;

  const String name_;
  const String config_prefix_;
  const bool is_template_;

  typedef OPENDDS_MAP(DomainParticipantImpl*, TransportImpl_rch) ParticipantMap;
  typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainMap;
  DomainMap domain_map_;
};

// Helper to turn a raw value into getters and setters.
template <typename Delegate, typename T>
class ConfigValue {
public:
  typedef void (Delegate::*Setter)(T);
  typedef T (Delegate::*Getter)(void) const;

  ConfigValue(Delegate& delegate,
              Setter setter,
              Getter getter)
    : delegate_(delegate)
    , setter_(setter)
    , getter_(getter)
  {}

  ConfigValue& operator=(T flag)
  {
    (delegate_.*setter_)(flag);
    return *this;
  }

  operator T() const
  {
    return (delegate_.*getter_)();
  }

  T get() const
  {
    return (delegate_.*getter_)();
  }

  ConfigValue& operator=(const ConfigValue& cv)
  {
    (delegate_.*setter_)(cv.get());
    return *this;
  }

private:
  Delegate& delegate_;
  Setter setter_;
  Getter getter_;
};

template <typename Delegate, typename T>
class ConfigValueRef {
public:
  typedef void (Delegate::*Setter)(const T&);
  typedef T (Delegate::*Getter)(void) const;

  ConfigValueRef(Delegate& delegate,
                 Setter setter,
                 Getter getter)
    : delegate_(delegate)
    , setter_(setter)
    , getter_(getter)
  {}

  ConfigValueRef& operator=(const T& flag)
  {
    (delegate_.*setter_)(flag);
    return *this;
  }

  operator T() const
  {
    return (delegate_.*getter_)();
  }

  T get() const
  {
    return (delegate_.*getter_)();
  }

  ConfigValueRef& operator=(const ConfigValueRef& cv)
  {
    (delegate_.*setter_)(cv.get());
    return *this;
  }

private:
  Delegate& delegate_;
  Setter setter_;
  Getter getter_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
