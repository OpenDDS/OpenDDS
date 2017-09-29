/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H
#define OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H

#include "../Versioned_Namespace.h"
#include <ostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct RestoreOutputStreamState {
  explicit RestoreOutputStreamState(std::ostream& o)
    : os_(o), state_(o.flags()) {}
  ~RestoreOutputStreamState() {
    os_.flags(state_);
  }
  std::ostream& os_;
  std::ios_base::fmtflags state_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H */
