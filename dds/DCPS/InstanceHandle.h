/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INSTANCEHANDLE_H
#define OPENDDS_DCPS_INSTANCEHANDLE_H

#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op_T.h>
#  include <ace/Thread_Mutex.h>
#endif /* ACE_HAS_CPP11 */

#include "dds/DdsDcpsInfrastructureC.h"

#include "dcps_export.h"

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
#ifdef ACE_HAS_CPP11
  std::atomic<int32_t> sequence_;
#else
  ACE_Atomic_Op<ACE_Thread_Mutex, long> sequence_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_INSTANCEHANDLE_H */
