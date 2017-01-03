// -*- C++ -*-

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

#include <vector>

namespace Test {

class Options;
class Writer;

class Publisher {
  public:
    /// Publication container.
    typedef std::vector< Writer*> PublicationVector;

    /// Construct with option information.
    Publisher( const Options& options);

    ~Publisher();

    /// Execute the test.
    void run();

    /// Gather the results.
    int status() const;

  private:
    /// Status indicator.
    int status_;

    /// Test options.
    const Options& options_;

    /// DomainParticipant.
    DDS::DomainParticipant_var participant_;

    /// Publisher.
    DDS::Publisher_var publisher_;

    /// Publications.
    PublicationVector publications_;

    /// Blocking object for test synchronization.
    DDS::WaitSet_var waiter_;
};

} // End of namespace Test

