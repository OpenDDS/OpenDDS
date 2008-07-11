// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTSENDLISTENER_H
#define OPENDDS_DCPS_TRANSPORTSENDLISTENER_H

#include "dds/DCPS/dcps_export.h"

class ACE_Message_Block;


namespace OpenDDS
{

  namespace DCPS
  {

    struct DataSampleListElement;


    class OpenDDS_Dcps_Export TransportSendListener
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

}  /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "TransportSendListener.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTSENDLISTENER_H */
