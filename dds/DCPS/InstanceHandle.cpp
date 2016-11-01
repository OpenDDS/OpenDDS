/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DdsDcpsGuidC.h"

#include "InstanceHandle.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

InstanceHandleGenerator::InstanceHandleGenerator(long begin)
  : sequence_(begin)
{
}

InstanceHandleGenerator::~InstanceHandleGenerator()
{
}

DDS::InstanceHandle_t
InstanceHandleGenerator::next()
{
  return ++sequence_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
