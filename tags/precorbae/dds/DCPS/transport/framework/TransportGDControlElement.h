// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTGDCONTROLELEMENT_H
#define TAO_DCPS_TRANSPORTGDCONTROLELEMENT_H

#include  "dds/DCPS/dcps_export.h"
#include  "TransportDefs.h"
#include  "TransportQueueElement.h"

class ACE_Message_Block ;

namespace TAO
{

  namespace DCPS
  {

    class TAO_DdsDcps_Export TransportGDControlElement : public TransportQueueElement
    {
      public:

        TransportGDControlElement(ACE_Message_Block* msg_block);

        virtual ~TransportGDControlElement();

      protected:

        virtual bool requires_exclusive_packet() const;

        virtual RepoId publication_id() const;

        virtual const ACE_Message_Block* msg() const;


        virtual void release_element(bool dropped_by_transport);

        virtual void data_delivered();


      private:

        /// The control message.
        ACE_Message_Block* msg_;
    };

  }
}


#endif  /* TAO_DCPS_TRANSPORTGDCONTROLELEMENT_H */
