// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_SUBSCRIPTION_INSTANCE_H
#define TAO_DDS_DCPS_SUBSCRIPTION_INSTANCE_H

#include  "dcps_export.h"
#include  "ReceivedDataElementList.h"
#include  "InstanceState.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO 
{
  namespace DCPS 
  {
    class DataReaderImpl ;
   /**
     * @class SubscriptionInstance
     *
     * @brief Struct that has information about an instance and the instance
     *        sample list. 
     */
      class SubscriptionInstance
      {
      public:
        SubscriptionInstance(DataReaderImpl *reader) :
            instance_state_(reader, (::DDS::InstanceHandle_t)this),
            last_sequence_(0), rcvd_sample_(&instance_state_)
        {
        }

        /// Instance state for this instance
        InstanceState instance_state_ ;

        /// sequence number of the move recect data sample received
        ACE_INT16 last_sequence_ ;

        /// Data sample(s) in this instance
        ReceivedDataElementList rcvd_sample_ ;
      } ;
  }
}

#endif /* TAO_DDS_DCPS_SUBSCRIPTION_INSTANCE_H */

