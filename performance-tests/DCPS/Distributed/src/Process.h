// -*- C++ -*-
// $Id$

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

#include <map>

namespace Test {

class Options;
class Writer;
class Reader;

class Process {
  public:
    /// Publication container.
    typedef std::map< std::string, Writer*> PublicationMap;

    /// Subscription container.
    typedef std::map< std::string, Reader*> SubscriptionMap;

    /// Construct with option information.
    Process( const Options& options);

    /// Destructor.
    ~Process();

    /// Execute the test.
    void run();

  private:
    /// Test options.
    const Options& options_;

    /// Test transport.
    OpenDDS::DCPS::TransportImpl_rch transport_;

    /// DomainParticipant.
    DDS::DomainParticipant_var participant_;

    /// Topic.
    DDS::Topic_var topic_;

    /// Publisher.
    DDS::Publisher_var publisher_;

    /// Publications.
    PublicationMap publications_;

    /// Blocking object for publication synchronization.
    DDS::WaitSet_var publicationWaiter_;

    /// Subscriber.
    DDS::Subscriber_var subscriber_;

    /// Subscriptions.
    SubscriptionMap subscriptions_;

    /// Blocking object for subscription synchronization.
    DDS::WaitSet_var subscriptionWaiter_;
};

} // End of namespace Test

