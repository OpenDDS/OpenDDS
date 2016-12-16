// -*- C++ -*-

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

#include <map>
#include <iosfwd>

namespace Test
{
  class Subscriber;
}

std::ostream& operator<<(std::ostream& str, const Test::Subscriber& value);

namespace Test
{

class Options;
class DataReaderListener;

class Subscriber {
  public:
    /// Construct with option information.
    Subscriber( const Options& options);

    ~Subscriber();

    /// Execute the test.
    void run();

    /// Total number of messages received by the subscriber.
    int total_messages() const;

    /// Valid number of messages received by the subscriber.
    int valid_messages() const;

    /// Number of samples received during the test from each writer.
    const std::map< long, long>& counts() const;

    /// Number of payload bytes received during the test from each writer.
    const std::map< long, long>& bytes() const;

    /// Priority of  writers.
    const std::map< long, long>& priorities() const;

    /// Format and dump raw data to a stream.
    std::ostream& rawData( std::ostream& str) const;

    /// Stream out statistics values.
    friend std::ostream& ::operator<<(std::ostream& str, const Subscriber& value);

  private:
    /// Test options.
    const Options& options_;

    /// DomainParticipant.
    DDS::DomainParticipant_var participant_;

    /// Topic.
    DDS::Topic_var topic_;

    /// Subscriber.
    DDS::Subscriber_var subscriber_;

    /// Reader.
    OpenDDS::DCPS::DataReaderEx_var reader_;

    /// Reader listener.
    DataReaderListener* listener_;

    /// Reader listener lifetime management.
    OpenDDS::DCPS::LocalObject_var safe_listener_;

    /// Blocking object for test synchronization.
    DDS::WaitSet_var waiter_;

    /// Blocking condition for test synchronization.
    DDS::StatusCondition_var status_;
};

} // End of namespace Test

