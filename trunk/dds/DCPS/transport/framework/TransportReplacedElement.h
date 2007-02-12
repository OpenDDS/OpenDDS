// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTREPLACEDELEMENT_H
#define TAO_DCPS_TRANSPORTREPLACEDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"
#include "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {
    class TransportReplacedElement;

    typedef Cached_Allocator_With_Overflow<TransportReplacedElement, ACE_SYNCH_NULL_MUTEX> 
                                             TransportReplacedElementAllocator;

    class TAO_DdsDcps_Export TransportReplacedElement : public TransportQueueElement
    {
      public:

        TransportReplacedElement(TransportQueueElement* orig_elem,
                                 TransportReplacedElementAllocator* allocator = 0);
        virtual ~TransportReplacedElement();

        /// Accessor for the publisher id.
        virtual RepoId publication_id() const;

        /// Accessor for the ACE_Message_Block
        virtual const ACE_Message_Block* msg() const;


      protected:

        virtual void release_element(bool dropped_by_transport);


      private:

        /// Reference to TransportReplacedElement allocator.
        TransportReplacedElementAllocator* allocator_;

        /// The publication_id() from the original TransportQueueElement
        RepoId publisher_id_;

        /// A deep-copy of the msg() from the original TransportQueueElement.
        ACE_Message_Block* msg_;
    };

  }
}

#if defined (__ACE_INLINE__)
#include "TransportReplacedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTREPLACEDELEMENT_H */
