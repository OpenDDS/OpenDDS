/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeObject.h"
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<XTypes::TkNone>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_NONE);
  return ti;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
