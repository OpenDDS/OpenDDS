/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_INSTANCEHANDLE_H
#define DCPS_INSTANCEHANDLE_H

#include "ace/Atomic_Op_T.h"
#include "ace/Thread_Mutex.h"

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
  ACE_Atomic_Op<ACE_Thread_Mutex, long> sequence_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_INSTANCEHANDLE_H */
