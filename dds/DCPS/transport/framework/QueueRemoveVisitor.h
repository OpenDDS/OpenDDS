/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QUEUEREMOVEVISITOR_H
#define OPENDDS_DCPS_QUEUEREMOVEVISITOR_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "BasicQueueVisitor_T.h"
#include "TransportDefs.h"
#include "TransportQueueElement.h"

#include "ace/Message_Block.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportQueueElement;

class OpenDDS_Dcps_Export QueueRemoveVisitor
  : public BasicQueueVisitor<TransportQueueElement> {
public:

  explicit QueueRemoveVisitor(const TransportQueueElement::MatchCriteria& mc);

  virtual ~QueueRemoveVisitor();

  /// The BasicQueue<T>::accept_remove_visitor() method will call
  /// this visit_element_remove() method for each element in the queue.
  virtual int visit_element_remove(TransportQueueElement* element,
                                   int&                   remove);

  /// Accessor for the status.  Called after this visitor object has
  /// been passed to BasicQueue<T>::accept_remove_visitor().
  RemoveResult status() const;

  int removed_bytes() const;

private:

  /// Criteria object describing the Queue Element that needs to be removed.
  const TransportQueueElement::MatchCriteria& mc_;

  /// Holds the status of our visit.
  RemoveResult status_;

  size_t removed_bytes_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "QueueRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_QUEUEREMOVEVISITOR_H */
