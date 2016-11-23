/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS_main.h>

#include "dds/DCPS/StaticIncludes.h"

#include "TestCase.h"

namespace {

bool use_source_timestamp(false);

void
parse_args(int& argc, ACE_TCHAR** argv)
{
  ACE_Arg_Shifter shifter(argc, argv);

  while (shifter.is_anything_left()) {
    if (shifter.cur_arg_strncasecmp(ACE_TEXT("source")) == 0) {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("INFO: %N:%l: parse_args() -")
                 ACE_TEXT(" using SOURCE_TIMESTAMP\n")));
      use_source_timestamp = true;
      shifter.consume_arg();

    } else {
      shifter.ignore_arg();
    }
  }
}

} // namespace

DDS::ReturnCode_t
TestCase::init_datareader(DDS::DataReaderQos& qos,
                          DDS::DataReaderListener_ptr& /*listener*/,
                          DDS::StatusMask& /*status*/)
{
  qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

  if (use_source_timestamp) {
    qos.destination_order.kind =
      DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
  }

  return DDS::RETCODE_OK;
}

void
TestCase::init_i(int, ACE_TCHAR*[]) {
  publisher_  = new TestPublisherType(*this);
  subscriber_ = new TestSubscriberType(*this);
  publisher_->init_i();
  subscriber_->init_i();
}

void
TestCase::fini_i() {
  delete publisher_;
  delete subscriber_;
}

int
TestCase::test()
{
  wait_for_subscribers(); // wait for association

  DDS::InstanceHandle_t instance;

  DDS::Time_t t1 = { 10, 0 };
  DDS::Time_t t2 = {  0, 0 };

  TestMessage m1 = { 0, "FIRST"  };
  TestMessage m2 = { 0, "SECOND" };

  instance = publisher_->register_instance(m1);

  if (publisher_->write_w_timestamp(m1, instance, t1) != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: test() -")
                      ACE_TEXT(" unable to write sample!\n")), -1);
  }

  if (publisher_->write_w_timestamp(m2, instance, t2) != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: test() -")
                      ACE_TEXT(" unable to write sample!\n")), -1);
  }

  ACE_OS::sleep(5); // wait for delivery

  TestMessageSeq messages;
  DDS::SampleInfoSeq info;

  instance = subscriber_->lookup_instance(m1);

  DDS::ReturnCode_t error =
    subscriber_->take_instance(messages,
                                   info,
                                   1,
                                   instance,
                                   DDS::ANY_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (error != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: test() -")
                      ACE_TEXT(" unable to take next sample!\n")), -1);
  }

  // Verify first sample is ordered correctly
  if (use_source_timestamp) {
    return ACE_OS::strcmp(messages[0].message, "FIRST") == 0;

  } else {
    return ACE_OS::strcmp(messages[0].message, "SECOND") == 0;
  }
}

void
TestCase::wait_for_subscribers()
{
  publisher_->wait_for_subscribers();
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  parse_args(argc, argv);

  TestCase test;
  return test.run(argc, argv);
}
