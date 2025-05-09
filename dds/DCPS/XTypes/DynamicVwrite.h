/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H

#include <dds/DCPS/Definitions.h>

#if !OPENDDS_CONFIG_SAFETY_PROFILE
#  include <dds/DCPS/ValueWriter.h>
#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export
bool vwrite(ValueWriter& vw, DDS::DynamicData_ptr value);

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H
