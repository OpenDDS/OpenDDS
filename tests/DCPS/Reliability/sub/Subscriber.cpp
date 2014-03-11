/*
 * $Id$
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

#include "dds/DCPS/StaticIncludes.h"

using namespace examples::boilerplate;

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = -1;
  try {
    // Initialize DomainParticipantFactory, handling command line args
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create domain participant
    DDS::DomainParticipant_var participant = createParticipant(dpf);

    // Register type support and create topic
    DDS::Topic_var topic = createTopic(participant);

    // Create subscriber
    DDS::Subscriber_var subscriber = createSubscriber(participant);

    std::string take_next = "-take-next";
    std::string take_seq  = "-take";
    std::string zero_copy = "-zero-copy";

    // Create Listener
    DataReaderListenerImpl* listener_impl;
    if (argc > 1) {
      if (take_next == argv[1]) {
        listener_impl = new TakeNextReaderListenerImpl;
      } else if (take_seq == argv[1]) {
        listener_impl = new SeqReaderListenerImpl;
      } else if (zero_copy == argv[1]) {
        listener_impl = new ZeroCopyReaderListenerImpl;
      }
    }
    if (!listener_impl) {
      listener_impl = new TakeNextReaderListenerImpl;
    }

    DDS::DataReaderListener_var listener(listener_impl);

    // Create DataReader with the listener attached
    DDS::DataReader_var reader = createDataReader(subscriber,
                                                  topic,
                                                  listener);

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
