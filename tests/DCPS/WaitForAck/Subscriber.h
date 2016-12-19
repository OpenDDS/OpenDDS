// -*- C++ -*-

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

#include <map>
#include <iosfwd>

namespace Test { class Subscriber; }

namespace Test {

class Options;

class Subscriber {
  public:
    /// Construct with option information.
    Subscriber( const Options& options);

    ~Subscriber();

    /// Execute the test.
    void run();

  private:
    /// Test options.
    const Options& options_;

    /// DomainParticipant.
    DDS::DomainParticipant_var participant_;

    /// Two readers.
    OpenDDS::DCPS::DataReaderEx_var reader_[ 2];

    /// Blocking object for test synchronization.
    DDS::WaitSet_var waiter_;

    /// Blocking condition for test synchronization.
    DDS::StatusCondition_var status_;
};

} // End of namespace Test

