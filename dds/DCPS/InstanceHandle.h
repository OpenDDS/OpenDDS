/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INSTANCEHANDLE_H
#define OPENDDS_DCPS_INSTANCEHANDLE_H

#include "dcps_export.h"
#include "Atomic.h"

#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const DDS::InstanceHandle_t HANDLE_UNKNOWN(0);

class OpenDDS_Dcps_Export InstanceHandleGenerator {
public:
  explicit InstanceHandleGenerator(long begin = HANDLE_UNKNOWN);

  ~InstanceHandleGenerator();

  DDS::InstanceHandle_t next();

private:
  Atomic<long> sequence_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_INSTANCEHANDLE_H */
