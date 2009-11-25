// -*- C++ -*-
//
// $Id$
#ifndef MONITORTASK_H
#define MONITORTASK_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsInfrastructureTypeSupportC.h"
#include "dds/DCPS/GuardCondition.h"
#include "dds/DCPS/WaitSet.h"

#include <ace/Task.h>

namespace Monitor {

class Options;
class MonitorData;

/**
 * @class MonitorTask
 *
 * @brief Asynchronous processing to receive and manage instrumentation data.
 *
 * This class provides a separate thread on which DDS instrumentation
 * data is received and forwarded to the GUI model.  The access model
 * simply serializes access to the processing so that only a single
 * thread will be executing the service interfaces at a time.  No finer
 * granularity is required for this application.  This is all based on a
 * single thread being used to process the asynchronous DDS data.
 */
class MonitorTask : public ACE_Task_Base {
  public:
    MonitorTask(
      MonitorData* data,
      const Options& options
    );

    virtual ~MonitorTask();

    /// @name Task_Base interfaces.
    /// @{
    virtual int open(void*);
    virtual int svc();
    virtual int close( u_long flags = 0);
    /// @}

    /// @name Thread control.
    /// @{
    void start();
    void stop();
    /// @}

    /// @name Messages from GUI.
    /// @{

    /// Establish a binding to a repository.  Clear any previously
    /// created structures first: there can be only one.
    void setRepoIor( const std::string& ior);

    /// @}

  private:
    enum ControlContext { InternalControl, ExternalControl};

    /// Terminate current processing.
    void shutdownRepo();

    /// Thread state flag.
    bool opened_;

    /// Service control flag.
    bool done_;

    /// Configuration Information.
    const Options& options_;

    /// Model data interface.
    MonitorData* data_;

    /// Id of the activated thread.
    ACE_thread_t thread_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;

    /// Condition to gate access to the service facilities.
    ACE_Condition<ACE_SYNCH_MUTEX> gate_;

    /// Context of the current processing used to serialize access.
    ControlContext controlContext_;

    /// Object to wait on DDS service conditions with.
    DDS::WaitSet_var waiter_;

    /// Asynchronous condition to programmatically trigger WaitSets.
    DDS::GuardCondition_var guardCondition_;

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

#endif /* MONITORTASK_H */

