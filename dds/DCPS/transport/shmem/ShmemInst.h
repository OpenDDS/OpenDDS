/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMINST_H
#define OPENDDS_SHMEMINST_H

#include "Shmem_Export.h"
#include "ShmemTransport.h"

#include "dds/DCPS/transport/framework/TransportInst.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Shmem_Export ShmemInst : public TransportInst {
public:

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  virtual OPENDDS_STRING dump_to_str() const;

  /// Size (in bytes) of the single shared-memory pool allocated by this
  /// transport instance.  Defaults to 16 megabytes.
  size_t pool_size_;

  /// Size (in bytes) of the control area allocated for each data link.
  /// This allocation comes out of the shared-memory pool defined by pool_size_.
  /// Defaults to 4 kilobytes.
  size_t datalink_control_size_;

  bool is_reliable() const { return true; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;

  const std::string& hostname() const { return hostname_; }
  const std::string& poolname() const { return poolname_; }

private:
  friend class ShmemType;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit ShmemInst(const std::string& name);

  TransportImpl_rch new_impl();
  std::string hostname_;
  std::string poolname_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_SHMEMINST_H */

