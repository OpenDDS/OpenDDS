// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTSENDELEMENT_H
#define TAO_DCPS_TRANSPORTSENDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"

namespace TAO
{

  namespace DCPS
  {
    //Yan struct DataSampleListElement;


    class TAO_DdsDcps_Export TransportSendElement : public TransportQueueElement
    {
      public:

        TransportSendElement(int                    initial_count,
                             DataSampleListElement* sample,
                             TransportSendElementAllocator* allocator = 0);
        virtual ~TransportSendElement();

        /// Accessor for the publisher id.
        virtual RepoId publication_id() const;

        /// Accessor for the ACE_Message_Block
        virtual const ACE_Message_Block* msg() const;


      protected:

        virtual void release_element(bool dropped_by_transport);


      private:

        /// This is the actual element that the transport framework was
        /// asked to send.
        DataSampleListElement* element_;

        /// Reference to TransportSendElement allocator.
        TransportSendElementAllocator* allocator_;
    };

  }
}

#if defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTSENDELEMENT_H */
