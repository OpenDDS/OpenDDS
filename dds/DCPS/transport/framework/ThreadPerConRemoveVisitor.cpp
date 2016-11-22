/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ThreadPerConRemoveVisitor.h"
#include "TransportQueueElement.h"
#include "TransportRetainedElement.h"

#if !defined (__ACE_INLINE__)
#include "ThreadPerConRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadPerConRemoveVisitor::~ThreadPerConRemoveVisitor()
{
  DBG_ENTRY("ThreadPerConRemoveVisitor", "~ThreadPerConRemoveVisitor");
}

int
ThreadPerConRemoveVisitor::visit_element_remove(SendRequest* req,
                                                int& remove)
{
  DBG_ENTRY("ThreadPerConRemoveVisitor", "visit_element_remove");

  TransportQueueElement::MatchOnDataPayload modp(this->sample_->rd_ptr());
  if ((req->op_ == SEND) && modp.matches(*req->element_)) {
    // We are visiting the element that we want to remove, since the
    // element "matches" our sample_.

    // In order to have the BasicQueue<T> remove the element that we
    // are currently visiting, set the remove flag to true (1).  The
    // BasicQueue<T> will perform the actual removal once we return
    // from this method.
    remove = 1;

    // Inform the element that we've made a decision - and it is
    // data_dropped()
    const bool released = req->element_->data_dropped();

    // Adjust our status_ to indicate that we actually found (and removed)
    // the sample.
    this->status_ = released ? REMOVE_RELEASED : REMOVE_FOUND;

    // Stop visitation since we've handled the element that matched
    // our sample_.
    return 0;
  }

  // Continue visitation.
  return 1;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
