// -*- C++ -*-

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

namespace Test {

class Options;
class DataWriterListener;

class Publisher {
  public:
    /// Construct with option information.
    Publisher( const Options& options);

    ~Publisher();

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

    /// Writer.
    DDS::DataWriter_var writer_[2];

    /// Blocking object for test synchronization.
    DDS::WaitSet_var waiter_;

    /// Blocking condition for test synchronization.
    DDS::StatusCondition_var status_[2];
};

} // End of namespace Test

