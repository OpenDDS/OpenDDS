/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMINST_H
#define OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMINST_H

#include "Shmem_Export.h"
#include "ShmemTransport.h"

#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/TimeDuration.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Shmem_Export ShmemInst : public TransportInst {
public:
  static const TimeDuration default_association_resend_period;

  virtual OPENDDS_STRING dump_to_str(DDS::DomainId_t domain) const;

  /// Size (in bytes) of the single shared-memory pool allocated by this
  /// transport instance.  Defaults to 16 megabytes.
  ConfigValue<ShmemInst, size_t> pool_size_;
  void pool_size(size_t ps);
  size_t pool_size() const;

  /// Size (in bytes) of the control area allocated for each data link.
  /// This allocation comes out of the shared-memory pool defined by pool_size_.
  /// Defaults to 4 kilobytes.
  ConfigValue<ShmemInst, size_t> datalink_control_size_;
  void datalink_control_size(size_t dcs);
  size_t datalink_control_size() const;

  bool is_reliable() const { return true; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info,
                                  ConnectionInfoFlags flags,
                                  DDS::DomainId_t domain) const;

  void hostname(const String& h);
  String hostname() const;

  const std::string& poolname() const { return poolname_; }

  void association_resend_period(const TimeDuration& arp);
  TimeDuration association_resend_period() const;

private:
  friend class ShmemType;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit ShmemInst(const std::string& name);

  TransportImpl_rch new_impl(DDS::DomainId_t domain);
  std::string poolname_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_SHMEMINST_H */
