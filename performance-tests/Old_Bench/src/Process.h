#ifndef PROCESS_H
#define PROCESS_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/GuardCondition.h"
#include "dds/DCPS/WaitSet.h"

#include "ace/Condition_T.h"
#include "ace/Synch_Traits.h"

#include <string>
#include <map>
#include <iosfwd>

namespace Test {

class Options;
class Publication;
class Subscription;

class Process {
  public:
    /// Construct with option information.
    Process( const Options& options);

    ~Process();

    /// Execute the test.
    void run();

    /// Signal the internal condition so that we no longer are waiting.
    void unblock();

    /// Format and dump summary data to a stream.
    std::ostream& summaryData( std::ostream& str) const;

    /// Format and dump raw data to a stream.
    std::ostream& rawData( std::ostream& str) const;

  private:
    /// Participant container.
    typedef std::map<std::string, DDS::DomainParticipant_var> ParticipantMap;

    /// Topic container.
    typedef std::map<std::string, DDS::Topic_var> TopicMap;

    /// Output file container.
    typedef std::map<std::string, std::ofstream*> OutputFileMap;

    /// Publication container.
    typedef std::map<std::string, Publication*> PublicationMap;

    /// Subscription container.
    typedef std::map<std::string, Subscription*> SubscriptionMap;

    /// Test options.
    const Options& options_;

    /// DomainParticipants.
    ParticipantMap participants_;

    /// Typename of the only data type in the test framework.
    std::string dataTypeName_;

    /// Output files for summary and raw data.
    OutputFileMap outputFiles_;

    /// Topics.
    TopicMap topics_;

    /// Publications.
    PublicationMap publications_;

    /// Blocking object for publication synchronization.
    DDS::WaitSet_var publicationWaiter_;

    /// Number of associations that the publications will make
    unsigned int publicationsAssociations_;

    /// Subscriptions.
    SubscriptionMap subscriptions_;

    /// Blocking object for subscription synchronization.
    DDS::WaitSet_var subscriptionWaiter_;

    /// Mechanism to unblock the WaitSet on command.
    DDS::GuardCondition_var guardCondition_;

    /// Lock for our condition.
    ACE_SYNCH_MUTEX lock_;

    /// Condition for blocking the main thread.
    ACE_Condition<ACE_SYNCH_MUTEX> condition_;

    /// Flag indicating that we have been commanded to terminate.
    bool terminated_;
};

} // End of namespace Test

/// Stream out statistics values.
inline
std::ostream& operator<<( std::ostream& str, const Test::Process& value)
{
  return value.summaryData( str);
}

#endif /* PROCESS_H */
