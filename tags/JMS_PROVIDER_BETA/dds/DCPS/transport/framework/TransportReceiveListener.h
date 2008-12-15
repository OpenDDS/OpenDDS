// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H
#define OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H

#include "dds/DCPS/dcps_export.h"


namespace OpenDDS
{

  namespace DCPS
  {

    class ReceivedDataSample;


    class OpenDDS_Dcps_Export TransportReceiveListener
    {
      public:

        virtual ~TransportReceiveListener();

        virtual void data_received(const ReceivedDataSample& sample)  = 0;


      protected:

        TransportReceiveListener();
    };

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "TransportReceiveListener.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H */
