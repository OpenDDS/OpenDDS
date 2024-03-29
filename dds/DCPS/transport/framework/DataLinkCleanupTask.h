/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_DATALINKCLEANUPTASK_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_DATALINKCLEANUPTASK_H

#include "QueueTaskBase_T.h"
#include "DataLink.h"
#include "DataLink_rch.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportImpl;

/**
 * @class DataLinkCleanupTask
 *
 * @brief Active Object responsible for cleaning up DataLink resources.
 *
 */
class OpenDDS_Dcps_Export DataLinkCleanupTask : public QueueTaskBase <DataLink_rch> {
public:
  DataLinkCleanupTask();

  virtual ~DataLinkCleanupTask();

  /// Handle reconnect requests.
  virtual void execute(DataLink_rch& dl);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DATALINKCLEANUP_H */
