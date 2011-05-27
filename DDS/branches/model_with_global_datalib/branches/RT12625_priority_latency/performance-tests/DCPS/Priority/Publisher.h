// -*- C++ -*-
// $Id$

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/WaitSet.h"

#include <map>

namespace Test {

class Options;
class Writer;
class DataWriterListener;

class Publisher {
  public:
    /// Publication container.
    typedef std::map< std::string, Writer*> PublicationMap;

    /// Construct with option information.
    Publisher( const Options& options);

    /// Destructor.
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

    /// Writer used to synchronize start of publication.
    DDS::DataWriter_var writer_;

    /// Publications.
    PublicationMap publications_;

    /// Blocking object for test synchronization.
    DDS::WaitSet_var waiter_;

    /// Blocking condition for test synchronization.
    DDS::StatusCondition_var status_;
};

} // End of namespace Test

