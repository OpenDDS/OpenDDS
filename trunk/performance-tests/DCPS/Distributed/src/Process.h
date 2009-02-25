// -*- C++ -*-
// $Id$
#ifndef PROCESS_H
#define PROCESS_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/WaitSet.h"

#include <map>
#include <iosfwd>

namespace Test {

class Options;
class Publication;
class Subscription;

class Process {
  public:
    /// Participant container.
    typedef std::map< std::string, DDS::DomainParticipant_var> ParticipantMap;

    /// Topic container.
    typedef std::map< std::string, DDS::Topic_var> TopicMap;

    /// Publication container.
    typedef std::map< std::string, Publication*> PublicationMap;

    /// Subscription container.
    typedef std::map< std::string, Subscription*> SubscriptionMap;

    /// Construct with option information.
    Process( const Options& options);

    /// Destructor.
    ~Process();

    /// Execute the test.
    void run();

    /// Format and dump raw data to a stream.
    std::ostream& rawData( std::ostream& str) const;

    /// Format and dump summary data to a stream.
    std::ostream& summaryData( std::ostream& str) const;

  private:
    /// Test options.
    const Options& options_;

    /// DomainParticipants.
    ParticipantMap participants_;

    /// Topics.
    TopicMap topics_;

    /// Publications.
    PublicationMap publications_;

    /// Blocking object for publication synchronization.
    DDS::WaitSet_var publicationWaiter_;

    /// Subscriptions.
    SubscriptionMap subscriptions_;

    /// Blocking object for subscription synchronization.
    DDS::WaitSet_var subscriptionWaiter_;
};

} // End of namespace Test

/// Stream out statistics values.
inline
std::ostream& operator<<( std::ostream& str, const Test::Process& value)
{
  return value.summaryData( str);
}

#endif /* PROCESS_H */

