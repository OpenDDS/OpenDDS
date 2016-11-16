/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BASICQUEUE_T_H
#define OPENDDS_DCPS_BASICQUEUE_T_H

#include "BasicQueueLinkPool_T.h"
#include "BasicQueueVisitor_T.h"
#include "dds/DCPS/PoolAllocationBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class BasicQueue : public PoolAllocationBase {
private:

  typedef BasicQueueLinkPool<T> PoolType;
  typedef BasicQueueLink<T>     LinkType;
  typedef BasicQueueVisitor<T>  VisitorType;

public:

  BasicQueue(size_t links_per_pool, size_t initial_pools)
  : head_(0),
    tail_(0),
    size_(0),
    pool_(links_per_pool, initial_pools)
  {}

  ~BasicQueue() {
  }

  /// Put a pointer to an element (T*) on to the queue.
  int put(T* elem) {
    // Get a "new" link from the pool.
    LinkType* link = pool_.obtain(elem);

    if (link == 0) {
      // Shouldn't happen unless we run out of memory (or encounter
      // an internal error condition of some sort, making the
      // pool_ unable to provide us with a valid link).
      return -1;
    }

    // Insert the link on the the end of the queue.
    if (this->tail_ == 0) {
      // The tail_ is NULL only when the queue is empty.
      // Set both the head_ and tail_ to point to the "new" link.
      this->head_ = this->tail_ = link;

    } else {
      // There is at least one element in the queue.
      // Set the next() of the current tail_ link to point to
      // the "new" link.  Then set the tail_ to point to the
      // "new" link.
      this->tail_->next(link);
      this->tail_ = link;
    }

    // Increment our size_ by one.
    ++this->size_;

    return 0;
  }

  /// Peek at the element at the top of the queue.  This is just
  /// like the get() operation except that the queue remains intact.
  T* peek() const {
    if (this->head_ == 0) {
      // The queue is empty.  Return a NULL pointer (0).
      return 0;
    }

    return this->head_->elem();
  }

  void replace_head(T* value) {
    if (this->head_ != 0) {
      this->head_->elem(value);
    }
  }

  /// Extract the top element from the queue.  Returns 0 if there
  /// are no elements in the queue.
  T* get() {
    LinkType* link = this->head_;

    if (link == 0) {
      // The queue is empty.  Return a NULL pointer (0).
      return 0;
    }

    // Adjust the head_ (and possibly the tail_ if there is only
    // one link currently in the queue).
    this->head_ = link->next();

    if (this->head_ == 0) {
      // That was the only link in the queue.  Now that the head_
      // indicates that the queue is empty, we need to update the
      // tail_ accordingly because we know it was pointing to the
      // same link as the head_.
      this->tail_ = 0;
    }

    // Save off the pointer to the link's elem().
    T* elem = link->elem();

    // Release the link object back to the pool.
    this->pool_.release(link);

    // Decrement our size_ by one.
    --this->size_;

    // And finally return the elem.
    return elem;
  }

  /// Accessor for the current number of elements in the queue.
  size_t size() const {
    return this->size_;
  }

  /// Standard way to supply a visitor to the queue - this will
  /// invoke visit_element(T* element) on the supplied visitor object
  /// once for each element in this BasicQueue<T> object, in order.
  /// The visitor can stop visitation early by returning 0 from
  /// its visit_element(T* element) method.
  void accept_visitor(VisitorType& visitor) const {
    LinkType* link = this->head_;

    while (link != 0) {
      if (visitor.visit_element(link->elem()) == 0) {
        return;
      }

      link = link->next();
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
    LinkType* prev_link = 0;
    LinkType* link = this->head_;

    while (link != 0) {
      // Save off the next_link now, in case we end up removing
      // (and releasing) the current link via a call to our
      // remove_next() call further on.
      LinkType* next_link = link->next();

      T* element = link->elem();

      int remove = 0;

      int keep_going = visitor.visit_element_remove(element, remove);

      if (remove == 1) {
        this->remove_next(prev_link);

        // We shouldn't change the prev_link here, because it
        // has become the prev_link of the next_link since the
        // (middle-man) link was just removed.

      } else {
        // Set it up for visitation of the next link.
        // Adjust the prev_link here because we didn't remove
        // the link.
        prev_link = link;
      }

      link = next_link;

      if (keep_going == 0) {
        // We are done.
        return;
      }
    }
  }

  /// This kind of visitation may cause the visitor to replace
  /// the currently visited element with a new element.
  void accept_replace_visitor(VisitorType& visitor) {
    LinkType* link = this->head_;

    while (link != 0) {
      // The subtle difference between this visitation method
      // and the normal "accept_visitor" visitation method is
      // that we supply a reference to a pointer to an element
      // here rather than just a pointer to an element.  This
      // allows the visitor to "replace" the current element
      // with a new element.
      if (visitor.visit_element_ref(link->elem_ref()) == 0) {
        return;
      }

      link = link->next();
    }
  }

private:

  /// Used by our accept_remove_visitor() method when an element needs
  /// to be removed from this BasicQueue<T> object.  The element that
  /// is supposed to be removed is the next() link object of the
  /// supplied "prev" link object.  It's done this way so that the
  /// (singlely) linked-list can be stitched back together after the
  /// appropriate link has been removed.
  void remove_next(LinkType* prev) {
    // We are either removing the head_ or the prev->next().
    // Note that we assume that in this contect, head_ will not
    // be NULL (0).  And, if prev is not NULL (0), then we assume
    // that prev->next() is not NULL (0).
    //
    // Note that we also assume that the prev link (if not NULL),
    // is indeed a member of the queue!
    LinkType* link = (prev == 0) ? this->head_ : prev->next();

    // Handle the case where the queue only contains one link
    // (we can then assume that this is the link we are removing).
    if (this->head_ == this->tail_) {
      // We are removing the only link in the queue.
      this->head_ = this->tail_ = 0;

    } else if (link == this->head_) {
      // We are removing the head link, and we know that there
      // are other links still in the queue.
      this->head_ = this->head_->next();

    } else if (link == this->tail_) {
      // We are removing the tail link, and we know that there
      // are other links still in the queue.
      prev->next(0);
      this->tail_ = prev;

    } else {
      // We are removing a link that is neither the head_ link
      // nor the tail_ link.
      prev->next(link->next());
    }

    // Decrement our size by one
    --this->size_;

    // And finally, we can release the link back to the pool.
    this->pool_.release(link);
  }

  /// The first (oldest) link in the queue.
  LinkType* head_;

  /// The last (newest) link in the queue.
  LinkType* tail_;

  /// The number of links currently in the queue.
  size_t size_;

  /// The pool of precreated LinkType objects (a "self-growing"
  /// allocator is used by the pool).
  PoolType pool_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUE_T_H */
