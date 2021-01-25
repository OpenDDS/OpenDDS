/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_BASICQUEUE_T_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_BASICQUEUE_T_H

#include <deque>
#include <algorithm>
#include <iterator>

#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/PoolAllocator.h"
#include "BasicQueueVisitor_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


template <typename T>
class BasicQueue : public PoolAllocationBase {
private:

  typedef BasicQueueVisitor<T>  VisitorType;
  typedef OPENDDS_DEQUE(T*) QueueImpl;
  typedef typename QueueImpl::iterator iterator;
  typedef typename QueueImpl::const_iterator const_iterator;
  QueueImpl elements_;
public:
  /// Put a pointer to an element (T*) on to the queue.
  int put(T* elem) {
    elements_.push_back(elem);
    return 0;
  }

  /// Peek at the element at the top of the queue.  This is just
  /// like the get() operation except that the queue remains intact.
  T* peek() const {
    return elements_.size() ? elements_[0] : 0;
  }

  void replace_head(T* value) {
    if (elements_.size()) {
      elements_[0] = value;
    }
  }

  /// Extract the top element from the queue.  Returns 0 if there
  /// are no elements in the queue.
  T* get() {
    T* result = 0;
    if (elements_.size()) {
      result = elements_.front();
      elements_.pop_front();
    }
    return result;
  }

  /// Accessor for the current number of elements in the queue.
  size_t size() const {
    return elements_.size();
  }

  /// Standard way to supply a visitor to the queue - this will
  /// invoke visit_element(T* element) on the supplied visitor object
  /// once for each element in this BasicQueue<T> object, in order.
  /// The visitor can stop visitation early by returning 0 from
  /// its visit_element(T* element) method.
  void accept_visitor(VisitorType& visitor) const {
    for (const_iterator itr = elements_.begin();
         itr != elements_.end(); ++itr) {
      visitor.visit_element(*itr);
    }
  }

  /// Alternate way to supply a visitor to the queue - this will
  /// invoke visit_element(T* element, int& remove) on the supplied
  /// visitor object once for each element in this BasicQueue<T>
  /// object, in order.
  ///
  /// The remove argument is a flag that should be set to true (1)
  /// in the visitor's visit_element_remove(T* element, int& remove)
  /// method if the visitor decides that the element should be removed
  /// from the queue.  The remove flag is always set to false (0)
  /// prior to calling the visitor's visit_element_remove(T* element,
  /// int& remove) method.
  ///
  /// The visitor can stop visitation early by returning 0 from
  /// its visit_element_remove(T* element, int& remove) method.
  void accept_remove_visitor(VisitorType& visitor) {
    QueueImpl tmp;
    for (iterator itr = elements_.begin();
         itr != elements_.end(); ++itr) {

      int remove = 0;
      int keep_going = visitor.visit_element_remove(*itr, remove);
      if (!remove)
        tmp.push_back(*itr);
      if (keep_going == 0) {
        std::copy(++itr,elements_.end(), std::back_inserter(tmp));
        break;
      }
    }
    elements_.swap(tmp);
  }
  /// This kind of visitation may cause the visitor to replace
  /// the currently visited element with a new element.
  void accept_replace_visitor(VisitorType& visitor) {
    for (iterator itr = elements_.begin();
         itr != elements_.end(); ++itr) {
      if (visitor.visit_element_ref(*itr) == 0)
        return;
    }
  }


  void swap(BasicQueue& other)
  {
    elements_.swap(other.elements_);
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUE_T_H */
