/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TakeNextReaderListenerImpl.h"
#include "SeqReaderListenerImpl.h"
#include "ZeroCopyReaderListenerImpl.h"
#include "Boilerplate.h"
#include <dds/DCPS/Service_Participant.h>
#include <model/Sync.h>
#include <stdexcept>
#include <ctime>
#include <iostream>
#include <ace/Arg_Shifter.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace examples::boilerplate;

namespace
{
  bool take_next = true;
  bool take = false;
  bool zero_copy = false;
  bool keep_last_one = false;
  int sleep_time = 0;
  int num_sleeps = 0;

  void
  parse_args(int& argc, ACE_TCHAR** argv)
  {
    ACE_Arg_Shifter shifter(argc, argv);

    while (shifter.is_anything_left())
    {
      const ACE_TCHAR* arg;

      if ((arg = shifter.get_the_parameter(ACE_TEXT("-num_sleeps"))))
      {
        num_sleeps = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-sleep_secs"))))
      {
        sleep_time = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-take-next")) == 0)
      {
        take_next = true;
        shifter.consume_arg();
      }
      else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-take")) == 0)
      {
        take_next = false;
        take = true;
        shifter.consume_arg();
      }
      else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-zero-copy")) == 0)
      {
        take_next = false;
        zero_copy = true;
        shifter.consume_arg();
      }
      else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-keep-last-one")) == 0)
      {
        keep_last_one = true;
        shifter.consume_arg();
      }
      else
      {
        shifter.ignore_arg();
      }
    }
  }
} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = -1;
  try {
    // Initialize DomainParticipantFactory, handling command line args
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    parse_args(argc, argv);

    // Create Listener
    DataReaderListenerImpl* listener_impl = NULL;
    if (take_next) {
      listener_impl = new TakeNextReaderListenerImpl;
    } else if (take) {
      listener_impl = new SeqReaderListenerImpl;
    } else if (zero_copy) {
      listener_impl = new ZeroCopyReaderListenerImpl;
    } else {
      listener_impl = new TakeNextReaderListenerImpl;
    }

    listener_impl->set_num_sleeps(num_sleeps);
    listener_impl->set_sleep_length(sleep_time);

    // Create domain participant
    DDS::DomainParticipant_var participant = createParticipant(dpf);

    // Register type support and create topic
    DDS::Topic_var topic = createTopic(participant);

    // Create subscriber
    DDS::Subscriber_var subscriber = createSubscriber(participant);

    DDS::DataReaderListener_var listener(listener_impl);

    // Create DataReader with the listener attached
    DDS::DataReader_var reader = createDataReader(subscriber,
                                                  topic,
                                                  listener,
                                                  keep_last_one);

    std::cout << "Waiting for connection" << std::endl;
    {
      // Block until reader has associated with a writer
      // but is no longer associated with any writer
      OpenDDS::Model::ReaderSync rs(reader);
    }

    if (listener_impl->sample_count() == listener_impl->expected_count()) {
      std::cout << "Got all " << listener_impl->sample_count()
                << " samples" << std::endl;
      status = 0;
    } else {
      std::cout << "ERROR: Got " << listener_impl->sample_count()
                << " samples, expected " << listener_impl->expected_count()
                << std::endl;
    }
    // Clean-up!
    cleanup(participant, dpf);

    // Listener will be cleaned up when reader goes out of scope
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      err.what()), -1);
  } catch (std::string& msg) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      msg.c_str()), -1);
  }

  return status;
}
