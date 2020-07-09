/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H
#define OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H

// This file is used by opendds_idl, so it must be relative.
#include "../Versioned_Namespace.h"

#include <ostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Save and automatically restore various formatting properties of a
 * std::ostream. This should be used in custom operator<< overload functions
 * when changing these properties.
 */
class RestoreOutputStreamState {
public:
  explicit RestoreOutputStreamState(std::ostream& stream)
    : stream_(stream)
    , flags_(stream.flags())
    , fill_(stream.fill())
    , precision_(stream.precision())
    , width_(stream.width())
  {
  }

  ~RestoreOutputStreamState()
  {
    stream_.flags(flags_);
    stream_.fill(fill_);
    stream_.precision(precision_);
    stream_.width(width_);
  }

private:
  std::ostream& stream_;
  const std::ios_base::fmtflags flags_;
  const char fill_;
  const std::streamsize precision_;
  const std::streamsize width_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RESTORE_OUTPUT_STREAM_STATE_H */
