/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RCEVENTHANDLER_H
#define OPENDDS_RCEVENTHANDLER_H

#include "ace/Event_Handler.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Templated Reference counted handle to a pointer.
/// A non-DDS specific helper class.
class RcEventHandler : public ACE_Event_Handler {
public:

  RcEventHandler()
  {
    this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

  }

  void _add_ref()
  {
    this->add_reference();
  }

  void _remove_ref()
  {
    this->remove_reference();
  }

};



} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_RCEVENTHANDLER_H */
