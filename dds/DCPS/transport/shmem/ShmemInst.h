/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Shmem_Export ShmemInst : public TransportInst {
public:

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  virtual void dump(std::ostream& os);

  /// Size (in bytes) of the single shared-memory pool allocated by this
  /// transport instance.  Defaults to 16 megabytes.
  size_t pool_size_;

  /// Size (in bytes) of the control area allocated for each data link.
  /// This allocation comes out of the shared-memory pool defined by pool_size_.
  /// Defaults to 4 kilobytes.
  size_t datalink_control_size_;

  bool is_reliable() const { return true; }

private:
  friend class ShmemType;
  explicit ShmemInst(const std::string& name);

  ShmemTransport* new_impl(const TransportInst_rch& inst);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_SHMEMINST_H */
