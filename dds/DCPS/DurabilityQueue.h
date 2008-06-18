// -*- C++ -*-
//
// $Id$

#ifndef OPENDDS_DURABILITY_QUEUE_H
#define OPENDDS_DURABILITY_QUEUE_H

#include <ace/Unbounded_Queue.h>

#include <algorithm>

namespace OpenDDS
{
  namespace DCPS
  {

    /**
     * @class DurabilityQueue
     *
     * @brief Queue class that provides a means to reset the
     *        underlying @c ACE_Allocator.
     *
     * This class only exists to provide a means to reset the
     * allocator used by the @c ACE_Unbounded_Queue base class.  It
     * has a specific use case, namely to correctly support instances
     * created by a persistent allocator.  The allocator address may
     * change between process runs, meaning the allocator address
     * stored in the persistent @c ACE_Unbounded_Queue instance will
     * be invalid.  Use the @c set_allocator() method to reset the
     * allocator address before performing any operations that will
     * require use of the allocator (e.g. enqueuing new items).
     */
    template<typename T>
    class DurabilityQueue : public ACE_Unbounded_Queue<T>
    {
    public:

      DurabilityQueue (ACE_Allocator * allocator)
        : ACE_Unbounded_Queue<T> (allocator)
      {
      }

      DurabilityQueue (DurabilityQueue<T> const & rhs)
        : ACE_Unbounded_Queue<T> (rhs.allocator_)
      {
        // Copied from ACE_Unbounded_Queue<>::copy_nodes().
        for (ACE_Node<T> *curr = rhs.head_->next_;
             curr != rhs.head_;
             curr = curr->next_)
          if (this->enqueue_tail (curr->item_) == -1)
            this->delete_nodes ();
      }

      ~DurabilityQueue ()
      {
      }

      void operator= (DurabilityQueue<T> const & rhs)
      {
        DurabilityQueue tmp (rhs);
        this->swap (rhs);
      }

      /// Reset allocator
      void set_allocator (ACE_Allocator * allocator)
      {
        if (allocator == 0)
          allocator = ACE_Allocator::instance ();

        this->allocator_ = allocator;
      }

      void swap (DurabilityQueue<T> & rhs)
      {
        std::swap (this->head_, rhs.head_);
        std::swap (this->cur_size_, rhs.current_size_);
        std::swap (this->allocator_, rhs.allocator_);
      }
    };

  }
}

#endif  /* OPENDDS_DURABILITY_QUEUE_H */
