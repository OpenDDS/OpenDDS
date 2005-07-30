// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTSENDELEMENT_H
#define TAO_DCPS_TRANSPORTSENDELEMENT_H

#include  "TransportQueueElement.h"

namespace TAO
{

  namespace DCPS
  {

    struct DataSampleListElement;


    class TransportSendElement : public TransportQueueElement
    {
      public:

        TransportSendElement(int                    initial_count,
                             DataSampleListElement* sample);
        virtual ~TransportSendElement();

        /// Accessor for the publisher id.
        virtual RepoId publication_id() const;

        /// Accessor for the ACE_Message_Block
        virtual const ACE_Message_Block* msg() const;


      protected:

        virtual void release_element();


      private:

        /// This is the actual element that the transport framework was
        /// asked to send.
        DataSampleListElement* element_;
    };

  }
}

#if defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTSENDELEMENT_H */
