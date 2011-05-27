// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H
#define OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H

#include "BasicQueueLink_T.h"
#include "BasicQueueLinkAllocator_T.h"
#include "ace/Malloc_T.h"


namespace OpenDDS
{

  namespace DCPS
  {

    template <typename T>
    class BasicQueueLinkPool
    {
      private:

        typedef BasicQueueLinkAllocator<T> AllocatorType;


      public:

        typedef BasicQueueLink<T> LinkType;


        // TMB: links_per_pool == CHUNK_SIZE in Mike's code
        //      num_pools      == 1 in Mike's code.
        BasicQueueLinkPool(unsigned links_per_pool, unsigned num_pools)
          {
            this->allocator_ = new AllocatorType(links_per_pool, num_pools);
          }


        ~BasicQueueLinkPool()
          {
            delete this->allocator_;
          }


        LinkType* obtain(T* elem)
          {
            LinkType* link;

            ACE_NEW_MALLOC_RETURN(
              link,
              (LinkType*)this->allocator_->malloc(sizeof(LinkType)),
              LinkType(elem),
              0
            );

            return link;
          }


        void release(LinkType* link)
          {
            link->reset();
            this->allocator_->free(link);
          }


      private:

        ACE_Allocator* allocator_;
    };

  }

}

#endif  /* OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H */

// OLD VERSION BELOW FOR REFERENCE
#if 0
#ifndef OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H
#define OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H

#include "BasicQueueLink_T.h"

namespace OpenDDS
{

  namespace DCPS
  {

    template <typename T>
    class BasicQueueLinkPool
    {
      public:

        BasicQueueLinkPool()
          : num_links_(0),
            links_(0),
            next_(0)
          {
          }


        ~BasicQueueLinkPool()
          {
            // Delete our array of links
            delete [] this->links_;

            // Delete the next pool in the chain.
            delete this->next_;
          }


        void init(unsigned links_per_pool, unsigned num_pools)
          {
            this->num_links_ = links_per_pool;
            this->links_ = new BasicQueueLink<T>[links_per_pool];

            if (num_pools > 1)
              {
                this->next_ = new BasicQueueLinkPool<T>();
                this->next_->init(links_per_pool, num_pools - 1);
              }
          }


        /// Obtain a link object from the pool.
        BasicQueueLink<T>* obtain()
          {
            for (unsigned i = 0; i < this->num_links_; i++)
              {
                // Attempt to obtain the current link.  A return value of 1
                // indicates that the current link was available, and has
                // now been marked as unavailble so that we can return it.
                if (links_[i].obtain() == 1)
                  {
                    // Return a pointer to the link.
                    return &links_[i];
                  }
              }
//MJM: Eeew.  Use a slightly modified cached allocator.  I'll send and
//MJM: example.  This thing will take forEVER spining through all the
//MJM: links.  The cached allocators overlay the type being stored with
//MJM: a next pointer in which they maintain a freelist.  The pointer is
//MJM: no longer needed once the element is in use, so there is no
//MJM: pointer or marking overhead for the list.  It is important to not
//MJM: return things from other allocators to a pool, but that goes for
//MJM: _all_ allocators, don't it?  I have one from a previous project
//MJM: that extended the cache each time the free list was exhausted,
//MJM: which is the same behavior as this pool.

            // We get here when we couldn't find an available link.
            // Time to get the next_ pool in the chain to do it.

            // Make sure that we have a next_ pool, and if not, create one.
            if (this->next_ == 0)
              {
                // Just add one more pool to the chain (that's what the 1
                // argument means).
                this->next_ = new BasicQueueLinkPool<T>();
                this->next_->init(this->num_links_, 1);
              }

            // Delegate to the next_ chunk to obtain() a link.
            return this->next_->obtain();
          }


      private:

        /// The number of links in the links_ array.
        unsigned num_links_;

        /// The array of links.
        BasicQueueLink<T>* links_;

        /// The next pool of links in the chain.
        BasicQueueLinkPool<T>* next_;
    };

  }

}

#endif  /* OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H */
#endif
