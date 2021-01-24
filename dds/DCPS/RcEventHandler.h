/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RCEVENTHANDLER_H
#define OPENDDS_DCPS_RCEVENTHANDLER_H

#include "ace/Event_Handler.h"
#include "dds/Versioned_Namespace.h"
#include "RcObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Templated Reference counted handle to a pointer.
/// A non-DDS specific helper class.
class RcEventHandler
  : public ACE_Event_Handler
  , public virtual RcObject {
public:

  RcEventHandler()
  {
    this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

  }

  ACE_Event_Handler::Reference_Count add_reference()
  {
    RcObject::_add_ref();
    return 1;
  }

  ACE_Event_Handler::Reference_Count remove_reference()
  {
    RcObject::_remove_ref();
    return 1;
  }

};



} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_RCEVENTHANDLER_H */
