#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif
#include "MessengerTypeSupportImpl.h"

#include <cstdlib>
#include <iostream>
using namespace std;

int run_test(int argc, ACE_TCHAR *argv[])
{
  using namespace DDS;
  using namespace OpenDDS::DCPS;
  using namespace Messenger;
  WaitSet_var ws = new WaitSet;
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, ""); 
  Topic_var topic = dp->create_topic("MyTopic", ts->get_type_name(),
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                           DEFAULT_STATUS_MASK);
  TransportImpl_rch pub_tport =
    TheTransportFactory->create_transport_impl(1, AUTO_CONFIG);
  PublisherImpl* pub_impl = dynamic_cast<PublisherImpl*> (pub.in());
  pub_impl->attach_transport(pub_tport.in());
  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
                                             DEFAULT_STATUS_MASK);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
  TransportImpl_rch sub_tport =
    TheTransportFactory->create_transport_impl(2, AUTO_CONFIG);
  SubscriberImpl* sub_impl = dynamic_cast<SubscriberImpl*> (sub.in());
  sub_impl->attach_transport(sub_tport.in());
  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  DataReader_var dr = sub->create_datareader(topic, dr_qos, 0,
                                             DEFAULT_STATUS_MASK);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(dw_sc);

  ReturnCode_t ret = RETCODE_OK;
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 0;
  sample.name = "data_X";
  sample.nest.value = B;
  for (int i(0); i < 20; ++i, ++sample.key)
    {
      //replace the 'X' with a random letter
      sample.name.inout()[5] = static_cast<char>((rand() % 26) + 'A');
      ret = mdw->write(sample, HANDLE_NIL);
      if (ret != RETCODE_OK) return ret;
      if (!(i % 4)) //once in a while write more than 1 sample per instance
        {
          Message sample2(sample);
          sample2.nest.value = A;
          sample2.name.inout()[5] = static_cast<char>((rand() % 26) + 'A');
          ret = mdw->write(sample2, HANDLE_NIL);
          if (ret != RETCODE_OK) return ret;
          sample2.nest.value = C;
          ret = mdw->write(sample2, HANDLE_NIL);
          if (ret != RETCODE_OK) return ret;
        }
    }

  DDS::StringSeq empty_query_params;
  ReadCondition_var dr_qc = dr->create_querycondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE, "ORDER BY name, nest.value",
    empty_query_params);
  ws->attach_condition(dr_qc);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  Duration_t five_seconds = {5, 0};
  bool passed = true, done = false;
  while (!done)
    {
      ret = ws->wait(active, five_seconds);
      if (ret == RETCODE_TIMEOUT)
        {
          cout << "timeout ";
          done = true; //continue to try and read
        }
      else if (ret != RETCODE_OK)
        {
          passed = false;
          break;
        }
      cout << "wait returned" << endl;
      MessageSeq data;
      SampleInfoSeq info;
      ret = mdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_qc);
      if (ret == RETCODE_NO_DATA)
        {
          // fall-through
        }
      else if (ret != RETCODE_OK)
        {
          cout << "ERROR: take_w_condition returned " << ret << endl;
          passed = false;
          done = true;
        }
      else
        {
          char largest = 0;
          for (CORBA::ULong i(0); i < data.length(); ++i)
            {
              cout << "Info:\tinstance_handle = " << info[i].instance_handle <<
                "\tsample_rank = " << info[i].sample_rank << '\n';
              if (info[i].valid_data)
                {
                  cout << "Data:\tkey = " << data[i].key <<
                    " \tname = " << data[i].name <<
                    "\tnest.value = " << data[i].nest.value <<
                    '\n';
                  if (data[i].name[5] >= largest)
                    {
                      largest = data[i].name[5];
                    }
                  else
                    {
                      cout << "ERROR: data is not sorted for key: " <<
                        data[i].key << endl;
                      passed = false;
                    }
                }
              cout << endl;
            }
        }
    }

  MessageSeq data;
  SampleInfoSeq info;
  ret = mdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_qc);
  if (ret != RETCODE_NO_DATA)
    {
      cout << "WARNING: there is still data in the reader\n";
    }

  ws->detach_condition(dr_qc);
  dr->delete_readcondition(dr_qc);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  return passed ? 0 : 1;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);

  // cleanup
  TheTransportFactory->release();
  TheServiceParticipant->shutdown ();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
