// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTSENDLISTENER_H
#define TAO_DCPS_TRANSPORTSENDLISTENER_H

#include  "dds/DCPS/dcps_export.h"

class ACE_Message_Block;


namespace TAO
{

  namespace DCPS
  {

    struct DataSampleListElement;


    class TAO_DdsDcps_Export TransportSendListener
    {
      public:

        virtual ~TransportSendListener();

        virtual void data_delivered(DataSampleListElement* sample) = 0;
        virtual void data_dropped(DataSampleListElement* sample, bool dropped_by_transport)   = 0;

        virtual void control_delivered(ACE_Message_Block* sample);
        virtual void control_dropped(ACE_Message_Block* sample, bool dropped_by_transport);


      protected:

        TransportSendListener();
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportSendListener.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_TRANSPORTSENDLISTENER_H */
