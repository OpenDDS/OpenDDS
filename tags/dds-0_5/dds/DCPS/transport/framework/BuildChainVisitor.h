// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_BUILDCHAINVISTOR_H
#define TAO_DCPS_BUILDCHAINVISTOR_H

#include  "BasicQueueVisitor_T.h"

class ACE_Message_Block;


namespace TAO
{

  namespace DCPS
  {

    class TransportQueueElement;


    class BuildChainVisitor : public BasicQueueVisitor<TransportQueueElement>
    {
      public:

        BuildChainVisitor();
        virtual ~BuildChainVisitor();

        virtual int visit_element(TransportQueueElement* element);

        /// Accessor to extract the chain, leaving the head_ and tail_
        /// set to 0 as a result.
        ACE_Message_Block* chain();


      private:

        ACE_Message_Block* head_;
        ACE_Message_Block* tail_;
    };

  }

}


#if defined (__ACE_INLINE__)
#include "BuildChainVisitor.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_BUILDCHAINVISTOR_H */
