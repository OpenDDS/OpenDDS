// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_SENDREPONSELISTENER_H
#define OPENDDS_DCPS_SENDREPONSELISTENER_H

#include "dds/DCPS/dcps_export.h"
#include "TransportSendListener.h"

namespace OpenDDS { namespace DCPS {

    /**
     * @class SendResponseListener
     *
     * @brief Simple listener to discard response samples.
     *
     * This is a simple listener implementation used to release response
     * samples once they have been either delivered or dropped.  No
     * special actions are taken to distiguish between the two results.
     */
    class OpenDDS_Dcps_Export SendResponseListener
      : public TransportSendListener {
      public:
        virtual ~SendResponseListener();

        virtual void data_delivered(DataSampleListElement* sample);
        virtual void data_dropped(
                       DataSampleListElement* sample,
                       bool dropped_by_transport
                     );

        virtual void control_delivered(ACE_Message_Block* sample);
        virtual void control_dropped(
                       ACE_Message_Block* sample,
                       bool dropped_by_transport
                     );
    };

}}  // namespace OpenDDS::DCPS

#endif /* OPENDDS_DCPS_SENDRESPONSELISTENER_H */

