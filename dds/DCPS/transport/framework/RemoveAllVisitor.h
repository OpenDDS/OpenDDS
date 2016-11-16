/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REMOVEALLVISITOR_H
#define OPENDDS_DCPS_REMOVEALLVISITOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueueVisitor_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportQueueElement;

class OpenDDS_Dcps_Export RemoveAllVisitor : public BasicQueueVisitor<TransportQueueElement> {
public:

  RemoveAllVisitor();

  virtual ~RemoveAllVisitor();

  /// The BasicQueue<T>::accept_remove_visitor() method will call
  /// this visit_element_remove() method for each element in the queue.
  virtual int visit_element_remove(TransportQueueElement* element,
                                   int&                   remove);

  /// Accessor for the status.  Called after this visitor object has
  /// been passed to BasicQueue<T>::accept_remove_visitor().
  int status() const;

  int removed_bytes() const;

private:

  /// Holds the status of our visit.
  int status_;

  size_t removed_bytes_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "RemoveAllVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_REMOVEALLVISITOR_H */
