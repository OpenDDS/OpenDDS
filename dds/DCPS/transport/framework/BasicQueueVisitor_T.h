/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BASICQUEUEVISITOR_T_H
#define OPENDDS_DCPS_BASICQUEUEVISITOR_T_H

#include "ace/CORBA_macros.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class BasicQueueVisitor {
public:

  BasicQueueVisitor() {
  }

  virtual ~BasicQueueVisitor() {
  }

  /// This is the visit_element() method that will be called when the
  /// visitation method used is BasicQueue<T>::accept_visitor().
  ///
  /// Return 0 if visiting should stop, return 1 to continue visiting.
  virtual int visit_element(T* element) {
    ACE_UNUSED_ARG(element);
    return 0;
  }

  /// This is the visit_element_remove() method that will be called when the
  /// visitation method used is BasicQueue<T>::accept_remove_visitor().
  ///
  /// Return 0 if visiting should stop, return 1 to continue visiting.
  /// The remove is an "inout" argument that is always passed-in with
  /// a false (0) value, indicating that the link should not be
  /// removed from the queue as a result of this visit.  If the
  /// visit_remove() implementation decides that the link should be
  /// removed, then it must set the remove argument to true (1).
  /// By default, this method is implemented to just return 0 to
  /// stop the "remove visitation" immediately.  It doesn't modify
  /// the value of the remove argument.
  virtual int visit_element_remove(T* element, int& remove) {
    ACE_UNUSED_ARG(element);
    ACE_UNUSED_ARG(remove);
    return 0;
  }

  /// This is the visit_element_ref() method that will be called when the
  /// visitation method used is BasicQueue<T>::accept_replace_visitor().
  ///
  /// Return 0 if visiting should stop, return 1 to continue visiting.
  virtual int visit_element_ref(T*& element) {
    ACE_UNUSED_ARG(element);
    return 0;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUEVISITOR_T_H */
