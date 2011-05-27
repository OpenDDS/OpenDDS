// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTSENDCONTROLELEMENT_H
#define TAO_DCPS_TRANSPORTSENDCONTROLELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportDefs.h"
#include "TransportQueueElement.h"

class ACE_Message_Block ;

namespace TAO
{

  namespace DCPS
  {

    class TransportSendListener;

    class TransportSendControlElement;

    typedef Cached_Allocator_With_Overflow<TransportSendControlElement, ACE_SYNCH_NULL_MUTEX> 
                                             TransportSendControlElementAllocator;

    class TAO_DdsDcps_Export TransportSendControlElement : public TransportQueueElement
    {
      public:

        TransportSendControlElement(int                    initial_count,
                                    RepoId                 publisher_id,
                                    TransportSendListener* listener,
                                    ACE_Message_Block*     msg_block,
                                    TransportSendControlElementAllocator* allocator = 0);

        virtual ~TransportSendControlElement();

        /// Overriden to always return true for Send Control elements.
        virtual bool requires_exclusive_packet() const;

        /// Accessor for the publisher id.
        virtual RepoId publication_id() const;

        /// Accessor for the ACE_Message_Block
        virtual const ACE_Message_Block* msg() const;

        /// Is the element a "control" sample from the specified pub_id?
        virtual bool is_control(RepoId pub_id) const;


      protected:

        virtual void release_element(bool dropped_by_transport);


      private:

        /// The publisher of the control message
        RepoId publisher_id_;

        /// The TransportSendListener object to call back upon.
        TransportSendListener* listener_;

        /// The control message.
        ACE_Message_Block* msg_;

        /// Reference to the TransportSendControlElement 
        /// allocator.
        TransportSendControlElementAllocator* allocator_;
    };

  }
}

#if defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTSENDCONTROLELEMENT_H */
