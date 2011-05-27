// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_MARKED_DEFAULT_QOS_H
#define TAO_DDS_DCPS_MARKED_DEFAULT_QOS_H

#include  "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{
  namespace DCPS
  {
    /**
    * This class defines the marked default qos values and provides accessors 
    * for them. The marked default qos contains a special value that indicates 
    * the default qos obtained via get_default_XXX_qos() call with the entity
    * factory should be used. These values are used when the user wants to use 
    * the default qos provided by the entity factory.
    */
    class TAO_DdsDcps_Export Marked_Default_Qos
    {
    public:
      static ::DDS::DomainParticipantQos  marked_default_DomainParticipantQos ();
      static ::DDS::TopicQos              marked_default_TopicQos ();
      static ::DDS::DataWriterQos         marked_default_DataWriterQos ();
      static ::DDS::PublisherQos          marked_default_PublisherQos ();
      static ::DDS::DataReaderQos         marked_default_DataReaderQos ();
      static ::DDS::SubscriberQos         marked_default_SubscriberQos ();
      static ::DDS::DataWriterQos         marked_default_DataWriter_Use_TopicQos ();
      static ::DDS::DataReaderQos         marked_default_DataReader_Use_TopicQos ();
    };


    // These values ARE NOT the default value but is an indicator to use the default 
    // qos via get_default_XXX_qos() call.
    #define PARTICIPANT_QOS_DEFAULT     \
          TAO::DCPS::Marked_Default_Qos::marked_default_DomainParticipantQos ()
    #define TOPIC_QOS_DEFAULT           \
          TAO::DCPS::Marked_Default_Qos::marked_default_TopicQos ()
    #define PUBLISHER_QOS_DEFAULT       \
          TAO::DCPS::Marked_Default_Qos::marked_default_PublisherQos () 
    #define SUBSCRIBER_QOS_DEFAULT      \
          TAO::DCPS::Marked_Default_Qos::marked_default_SubscriberQos ()
    #define DATAWRITER_QOS_DEFAULT      \
          TAO::DCPS::Marked_Default_Qos::marked_default_DataWriterQos ()
    #define DATAREADER_QOS_DEFAULT      \
          TAO::DCPS::Marked_Default_Qos::marked_default_DataReaderQos ()
    #define DATAWRITER_QOS_USE_TOPIC_QOS    \
          TAO::DCPS::Marked_Default_Qos::marked_default_DataWriter_Use_TopicQos ()   
    #define DATAREADER_QOS_USE_TOPIC_QOS    \
          TAO::DCPS::Marked_Default_Qos::marked_default_DataReader_Use_TopicQos ()

  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_MARKED_DEFAULT_QOS_H */
