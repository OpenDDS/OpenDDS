/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_main.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#endif

#include "TestCase.h"


namespace {

OpenDDS::DCPS::TransportIdType local_transport_id(1);
ACE_TString local_transport_type(ACE_TEXT("SimpleTcp"));
const int num_messages = 100;

void
parse_args(int& argc, ACE_TCHAR** argv)
{
  ACE_Arg_Shifter shifter(argc, argv);

  while (shifter.is_anything_left()) {
    if (shifter.cur_arg_strncasecmp(ACE_TEXT("udp")) == 0) {
      local_transport_id = 2;
      local_transport_type = ACE_TEXT("udp");

      shifter.consume_arg();

    } else if (shifter.cur_arg_strncasecmp(ACE_TEXT("multicast")) == 0) {
      local_transport_id = 3;
      local_transport_type = ACE_TEXT("multicast");

      shifter.consume_arg();

    } else {
      shifter.ignore_arg();
    }
  }
}

} // namespace

DDS::ReturnCode_t
TestCase::init_transport(OpenDDS::DCPS::TransportIdType& transport_id,
                         ACE_TString& transport_type)
{
  // Ensure the same tranpsort ID is used for both publishers and
  // subscribers; this will force an association across the same
  // TransportImpl instance:
  transport_id = local_transport_id;
  transport_type = local_transport_type;

  return DDS::RETCODE_OK;
}


int
TestCase::test()
{
  wait_for_subscribers(); // wait for association

  // As there are no fully assoication establishment between pub and sub for UDP
  // transport, a delay is required for the test to receive all messages.
  if (local_transport_type == ACE_TEXT("udp")) {
    ACE_OS::sleep (2);
  }

  // Write test data to exercise the data paths:
  for (int i = 0; i < num_messages; ++i) {
    TestMessage message = { i, "Testing!" };
    if (this->writer_i_->write(message, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: test() -")
                        ACE_TEXT(" unable to write sample!\n")), -1);
    }
  }

  ACE_OS::sleep(5); // wait for delivery

  // This test verifies associations formed between subscribers and
  // publishers attached to the same TransportImpl. There is nothing
  // which needs to be verified other than the association is formed
  // without crashing the DCPS subsystem.
#if 1
  for (int i = 0; i < num_messages; ++i) {
    TestMessage message;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = this->reader_i_->take_next_sample(message, si) ;

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
 #endif
  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  parse_args(argc, argv);

  TestCase test;
  return test.run(argc, argv);
}
