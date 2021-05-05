#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/unique_ptr.h"

#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/WaitForSample.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

#include <memory>

using namespace Messenger;
using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[]){
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      DDS::DomainParticipant_var participant =
        dpf->create_participant(31,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(participant.in())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      MessageTypeSupportImpl::_var_type servant = new MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != servant->register_type(participant.in(), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = servant->get_type_name();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  topic_qos,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::Subscriber_var sub =
        participant->create_subscriber(sub_qos, DDS::SubscriberListener::_nil(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(sub.in())) {
        cerr << "create_subscriber failed." << endl;
        exit(1);
      }

      DDS::DataReader_var dr =
        sub->create_datareader(topic.in(),
                               DATAREADER_QOS_DEFAULT,
                               DDS::DataReaderListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      Messenger::MessageDataReader_var message_dr =
        Messenger::MessageDataReader::_narrow(dr.in());

      Messenger::Message message;
      DDS::SampleInfo si;
      bool valid_data = false;

      while (!valid_data) {

        Utils::waitForSample(dr);
        DDS::ReturnCode_t ret = message_dr->take_next_sample(message, si);

        if (ret != DDS::RETCODE_OK) {
          cerr << "take sample failed" << endl;
          exit(1);
        }

        if (si.valid_data) {
          stringstream ss;
          ss << "Subscriber " << ACE_OS::getpid() << " got new message data:" << endl;
          ss << " - From  : " << message.from << endl;
          ss << " - Count : " << message.count << endl;
          cout << ss.str() << flush;
          valid_data = true;
        }
      }

      message_dr = 0;

      sub->delete_datareader(dr);
      participant->delete_subscriber(sub);

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in());
  }
  catch (CORBA::Exception& e)
  {
    cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }
  TheServiceParticipant->shutdown();

  return 0;
}
