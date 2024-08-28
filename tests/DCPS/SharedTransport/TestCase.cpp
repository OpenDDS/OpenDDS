/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TestCase.h"

#include <tests/Utils/ExceptionStreams.h>

#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/TimePoint_T.h>
#include <dds/DCPS/WaitSet.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_unistd.h>

#include <iostream>

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

  qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
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
    (*pub)->wait_for_subscribers(static_cast<CORBA::Long>(count));
  }
}

int
TestCase::test()
{
  wait_for_subscribers(); // wait for association

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

  // wait for delivery
  for (TestSubscriberVector::iterator sub = subscribers_.begin();
       sub != subscribers_.end(); ++sub) {
    size_t read = 0;
    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::ReadCondition_var rc =
        (*sub)->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
                                     DDS::NEW_VIEW_STATE,
                                     DDS::ALIVE_INSTANCE_STATE);
    ws->attach_condition(rc);
    const OpenDDS::DCPS::MonotonicTimePoint start = OpenDDS::DCPS::MonotonicTimePoint::now();
    const OpenDDS::DCPS::TimeDuration timeout(30, 0);
    const DDS::Duration_t one_sec = {1, 0};
    const size_t num_expected = num_messages * publishers_.size();
    do {
      TestMessageSeq data_values;
      DDS::SampleInfoSeq sample_infos;
      const CORBA::Long expected = static_cast<CORBA::Long>(num_expected);
      (*sub)->read_w_condition(data_values, sample_infos, expected, rc);
      read += data_values.length();
      if (read != num_expected) {
        DDS::ConditionSeq active;
        DDS::ReturnCode_t ret = ws->wait(active, one_sec);
        if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_TIMEOUT) {
          ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: wait()")
                      ACE_TEXT(" ERROR: wait for samples failed: %d\n"),
                      ret), -1);
        }
      }
    } while (read != num_expected && (OpenDDS::DCPS::MonotonicTimePoint::now() - start < timeout));

    ws->detach_condition(rc);

    // Only check the number read if the transport is reliable
    if (read < num_expected) {
      ACE_ERROR_RETURN((LM_ERROR, "ERROR: timeout exceeded\n"), -1);
    }

  }

  // This test verifies associations formed between subscribers and
  // publishers attached to the same TransportImpl. There is nothing
  // which needs to be verified other than the association is formed
  // without crashing the DCPS subsystem.
  for (int j = 0; j < num_messages; ++j) {
    TestMessageSeq message_seq;
    DDS::SampleInfoSeq si_seq;
    ::CORBA::Long max_take = 1;

    // For each subscriber
    for (TestSubscriberVector::iterator sub = subscribers_.begin();
         sub != subscribers_.end(); ++sub) {
      DDS::ReadCondition_var tc = (*sub)->create_readcondition(DDS::READ_SAMPLE_STATE,
                                                               DDS::ANY_VIEW_STATE,
                                                               DDS::ALIVE_INSTANCE_STATE);
      // For each publisher
      size_t num_pubs = publishers_.size();
      for (unsigned int i = 0; i < num_pubs; ++i) {
        DDS::ReturnCode_t status = (*sub)->take_w_condition(message_seq, si_seq, max_take, tc);
        if (status == DDS::RETCODE_OK) {
          DDS::SampleInfo si = si_seq[0];
          TestMessage message = message_seq[0];
          std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
          std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

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
          // We can only be sure there will be something to read if the transport is reliable
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
    set_writers(ACE_OS::atoi(argv[1]));
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
  int ret = 1;
  try
  {
    TestCase test;
    ret = test.run(argc, argv);
  }
  catch (const CORBA::BAD_PARAM& ex)
  {
    ex._tao_print_exception("Exception caught in TestCase.cpp:");
    return 1;
  }
  return ret;
}
