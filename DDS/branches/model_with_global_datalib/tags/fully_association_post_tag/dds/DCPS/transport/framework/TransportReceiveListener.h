// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTRECEIVELISTENER_H
#define TAO_DCPS_TRANSPORTRECEIVELISTENER_H

#include  "dds/DCPS/dcps_export.h"


namespace TAO
{

  namespace DCPS
  {

    class ReceivedDataSample;


    class TAO_DdsDcps_Export TransportReceiveListener
    {
      public:

        virtual ~TransportReceiveListener();

        virtual void data_received(const ReceivedDataSample& sample)  = 0;


      protected:

        TransportReceiveListener();
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportReceiveListener.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_TRANSPORTRECEIVELISTENER_H */
