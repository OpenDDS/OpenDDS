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
#include "dds/DCPS/Service_Participant.h"


#include <ace/Task.h>

namespace Monitor {

class Options;
class MonitorDataStorage;

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
    friend class Viewer;
    friend class MonitorData;
    
    /// Alias the RepoKey type locally.
    typedef OpenDDS::DCPS::Service_Participant::RepoKey RepoKey;
    enum { DEFAULT_REPO = OpenDDS::DCPS::Service_Participant::DEFAULT_REPO };

    /// Map IOR values to repository key values.
    typedef std::map< std::string, RepoKey> IorKeyMap;

    MonitorTask(
      MonitorDataStorage* data,
      const Options&      options
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
    RepoKey setRepoIor( const std::string& ior);

    /// Set the active repository.
    bool setActiveRepo( RepoKey key);

    /// @}

    /// Provide access to the IOR mappings.
    const IorKeyMap& iorKeyMap() const;

  private:
    /// Specialize data handling by type.
    template< class ReaderType, typename DataType>
    void dataUpdate( DDS::DataReader_ptr reader);

    /// Specialize subscriptions by type support.
    template< class TypeSupport>
    void createSubscription(
           DDS::Subscriber_ptr subscriber,
           const char*         topicName,
           int                 type
         ); 

    /// Create subscriptions for Builtin Topics.
    void createBuiltinSubscription(
           const char* topicName,
           int         type
         );

    /// Terminate the current instrumentation processing.
    void stopInstrumentation();

    /// Dispatch a reader with data to be processed.
    void dispatchReader( DDS::DataReader_ptr reader);

    /// Thread state flag.
    bool opened_;

    /// Service control flag.
    bool done_;

    /// Flow control.
    bool inUse_;

    /// Configuration Information.
    const Options& options_;

    /// Model data storage interface.
    MonitorDataStorage* data_;

    /// Id of the activated thread.
    ACE_thread_t thread_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;

    /// Condition to gate access to the service facilities.
    ACE_Condition<ACE_SYNCH_MUTEX> gate_;

    /// Object to wait on DDS service conditions with.
    DDS::WaitSet_var waiter_;

    /// Asynchronous condition to programmatically trigger WaitSets.
    DDS::GuardCondition_var guardCondition_;

    /// Local Domain Participant
    DDS::DomainParticipant_var participant_;

    typedef std::map< DDS::InstanceHandle_t, int> HandleTypeMap;
    HandleTypeMap handleTypeMap_;

    /// Map IOR strings to repository key values.
    IorKeyMap iorKeyMap_;

    /// Key value of the currently active repository.
    RepoKey activeKey_;

    /// Is any repo ever set to active ?
    bool activeKeyInited_;
    
    /// Repository key value used for last IOR to be set.
    RepoKey lastKey_;
};

} // End of namespace Monitor

#endif /* MONITORTASK_H */

