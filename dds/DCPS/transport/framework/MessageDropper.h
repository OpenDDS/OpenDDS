/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_MESSAGE_DROPPER_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_MESSAGE_DROPPER_H

#include "dds/DCPS/ConfigStoreImpl.h"
#include "dds/DCPS/dcps_export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MessageDropper {
public:
  MessageDropper()
    : drop_messages_(false)
    , drop_messages_m_(0)
    , drop_messages_b_(0)
  {}

  bool drop_messages() const { return drop_messages_; }
  double drop_messages_m() const { return drop_messages_m_; }
  double drop_messages_b() const { return drop_messages_b_; }

  void reload(RcHandle<ConfigStoreImpl> config_store,
              const String& config_prefix);

  bool should_drop(ssize_t length) const;

  bool should_drop(const iovec iov[],
                   int n,
                   ssize_t& length) const;

private:
  bool drop_messages_;
  double drop_messages_m_;
  double drop_messages_b_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_TRANSPORT_FRAMEWORK_MESSAGE_DROPPER_H
