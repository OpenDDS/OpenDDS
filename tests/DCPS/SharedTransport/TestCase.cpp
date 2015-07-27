/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_main.h>

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include "TestCase.h"
#include "tests/Utils/ExceptionStreams.h"


namespace {

const int num_messages = 100;

} // namespace

TestCase::TestCase()
: num_writers_(1),
  num_readers_(1)
{
}

DDS::ReturnCode_t
TestCase::init_datawriter(
  DDS::DataWriterQos& qos,
  DDS::DataWriterListener_ptr&,
  DDS::StatusMask&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: TestCase::init_datawriter\n")));

  qos.liveliness.lease_duration.sec = 1;
  qos.liveliness.lease_duration.nanosec = 0;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
TestCase::init_datareader(
  DDS::DataReaderQos& qos,
  DDS::DataReaderListener_ptr&,
  DDS::StatusMask&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: TestCase::init_datareader\n")));

  qos.liveliness.lease_duration.sec = 1;
  qos.liveliness.lease_duration.nanosec = 0;
  return DDS::RETCODE_OK;
}

void TestCase::wait_for_subscribers()
{
  // Count the number of subscribers
  size_t count = subscribers_.size();

  for (TestPublisherVector::iterator pub = publishers_.begin();
       pub != publishers_.end(); ++pub) {
    (*pub)->wait_for_subscribers(count);
  }
}

int
TestCase::test()
{
  wait_for_subscribers(); // wait for association

  // As there are no fully association establishment between pub and sub for UDP
  // transport, a delay is required for the test to receive all messages.
  ACE_OS::sleep (2);

  // Write test data to exercise the data paths:
  for (int i = 0; i < num_messages; ++i) {
    int pind = 0;
    for (TestPublisherVector::iterator pub = publishers_.begin();
         pub != publishers_.end(); ++pub) {
      TestMessage message = { (100*pind) + i, "Testing!" };
      if ((*pub)->write_message(message)) {
        return -1;
      }
      ++pind;
    }
  }

  ACE_OS::sleep(5); // wait for delivery

  // This test verifies associations formed between subscribers and
  // publishers attached to the same TransportImpl. There is nothing
  // which needs to be verified other than the association is formed
  // without crashing the DCPS subsystem.
  for (int i = 0; i < num_messages; ++i) {
    TestMessage message;
    DDS::SampleInfo si;

    // For each subscriber
    for (TestSubscriberVector::iterator sub = subscribers_.begin();
         sub != subscribers_.end(); ++sub) {
      // For each publisher
      size_t num_pubs = publishers_.size();
      for (unsigned int i = 0; i < num_pubs; ++i) {
        DDS::ReturnCode_t status = (*sub)->take_next_sample(message, si);
        if (status == DDS::RETCODE_OK) {
          std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
          std::cout << "SampleInfo.instance_state = " << si.instance_state << std::endl;

          if (si.valid_data) {
            std::cout << "Message: key    = " << message.key << std::endl
                      << "         message = " << message.message.in()   << std::endl;
          } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
            return -1;
          } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
            return -1;
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: take_next_sample()")
                        ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                        si.instance_state), -1);
          }
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: take_next_sample()")
                      ACE_TEXT(" ERROR: unexpected status: %d\n"),
                      status), -1);
        }
      }
    }
  }
  return 0;
}

void
TestCase::init_i(int argc, ACE_TCHAR* argv[])
{
  if (argc > 1) {
    set_writers(atoi(argv[1]));
  }

  // Create publishers
  for (int i = 0; i < num_writers_; ++i) {
    TestPublisherType* mdw = new TestPublisherType(*this);
    mdw->init_i();
    publishers_.push_back(mdw);
  }

  // Create subscribers
  for (int j = 0; j < num_readers_; ++j) {
    TestSubscriberType* mdr = new TestSubscriberType(*this);
    mdr->init_i();
    subscribers_.push_back(mdr);
  }
}

void
TestCase::fini_i()
{
  TestPublisherVector::iterator pub;
  TestSubscriberVector::iterator sub;

  for (pub = publishers_.begin(); pub != publishers_.end(); ++pub) {
    (*pub)->fini_i();
    delete (*pub);
  }
  publishers_.erase(publishers_.begin(), publishers_.end());

  for (sub = subscribers_.begin(); sub != subscribers_.end(); ++sub) {
    (*sub)->fini_i();
    delete (*sub);
  }
  subscribers_.erase(subscribers_.begin(), subscribers_.end());
}

void
TestCase::set_writers(int count)
{
  if (count > 0) {
    num_writers_ = count;
  }
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  TestCase test;
  return test.run(argc, argv);
}
