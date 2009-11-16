/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef MONITORDATA_H
#define MONITORDATA_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsInfrastructureTypeSupportC.h"

namespace Monitor {

class MonitorData {
  public:
    /// Construct with an IOR only.
    MonitorData( const std::string& ior);

    /// Virtual destructor.
    virtual ~MonitorData();

  private:
    /// Read and process any existing samples.
    void processCurrentSamples(
           ::DDS::DataReaderListener_var                    listener,
           ::DDS::PublicationBuiltinTopicDataDataReader_var reader
         );

    /// Local Domain Participant
    ::DDS::DomainParticipant_var participant_;

    /// Subscriber for the Builtin topics.
    ::DDS::Subscriber_var builtinSubscriber_;

    /// DataReader for the BuiltinTopic "DCPSParticipant".
    ::DDS::ParticipantBuiltinTopicDataDataReader_var participantReader_;

    /// DataReader for the BuiltinTopic "DCPSTopic".
    ::DDS::TopicBuiltinTopicDataDataReader_var topicReader_;

    /// DataReader for the BuiltinTopic "DCPSPublication".
    ::DDS::PublicationBuiltinTopicDataDataReader_var publicationReader_;

    /// DataReader for the BuiltinTopic "DCPSSubscription".
    ::DDS::SubscriptionBuiltinTopicDataDataReader_var subscriptionReader_;
};

} // End of namespace Monitor

#endif /* MONITORDATA_H */

