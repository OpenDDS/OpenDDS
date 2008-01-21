// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_BUILDCHAINVISTOR_H
#define OPENDDS_DCPS_BUILDCHAINVISTOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueueVisitor_T.h"

class ACE_Message_Block;


namespace OpenDDS
{

  namespace DCPS
  {

    class TransportQueueElement;


    class OpenDDS_Dcps_Export BuildChainVisitor : public BasicQueueVisitor<TransportQueueElement>
    {
      public:

        BuildChainVisitor();
        virtual ~BuildChainVisitor();

        // The using declaration is added to resolve the "hides virtual functions"
        // compilation warnings on Solaris.
        using BasicQueueVisitor<TransportQueueElement>::visit_element;

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


#endif  /* OPENDDS_DCPS_BUILDCHAINVISTOR_H */
