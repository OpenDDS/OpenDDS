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
#include <iostream>
using namespace std;

void received_data (const Messenger::MessageSeq& data,
                    Messenger::MessageDataWriter_ptr mdw,
                    Messenger::Message& msg)
{
  for (CORBA::ULong j(0); j < data.length(); ++j)
    {
      cout << "took sample " << data[j].subject_id << endl;
      if (data[j].subject_id > 8) mdw->dispose(msg, DDS::HANDLE_NIL);
    }
}

int run_test(int argc, ACE_TCHAR *argv[])
{
  using namespace DDS;
  using namespace OpenDDS::DCPS;
  using namespace Messenger;
  WaitSet_var ws = new WaitSet;
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0);
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, ts->get_type_name());
  Topic_var topic = dp->create_topic("MyTopic", ts->get_type_name(),
    TOPIC_QOS_DEFAULT, 0);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0);
  TransportImpl_rch pub_tport =
    TheTransportFactory->create_transport_impl(1, AUTO_CONFIG);
  PublisherImpl* pub_impl = dynamic_cast<PublisherImpl*> (pub.in());
  pub_impl->attach_transport(pub_tport.in());
  DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0);
  TransportImpl_rch sub_tport =
    TheTransportFactory->create_transport_impl(2, AUTO_CONFIG);
  SubscriberImpl* sub_impl = dynamic_cast<SubscriberImpl*> (sub.in());
  sub_impl->attach_transport(sub_tport.in());
  DataReader_var dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCH_STATUS);
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITY_SEC, DURATION_INFINITY_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(dw_sc);

  ReturnCode_t ret = RETCODE_OK;
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message msg = {0};
  for (int i(0); i < 12; ++i)
    {
      ++msg.subject_id;
      ret = mdw->write(msg, HANDLE_NIL);
      if (ret != RETCODE_OK) return ret;
    }

  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    NEW_VIEW_STATE, ALIVE_INSTANCE_STATE);
  ReadCondition_var dr_rc2 = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ws->attach_condition(dr_rc);
  ws->attach_condition(dr_rc2);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  bool passed = true, done = false;
  while (!done)
    {
      ret = ws->wait(active, infinite);
      if (ret != RETCODE_OK)
        {
          passed = false;
          break;
        }
      cout << "wait returned" << endl;
      for (CORBA::ULong i(0); i < active.length(); ++i)
        {
          if (active[i] == dr_rc)
            {
              // To test both take_w_condition and
              // take_next_instance_w_condition, we'll limit the "take" to 3
              // samples and then use take_next_instance_w_condition.
              MessageSeq data;
              SampleInfoSeq info;
              ret = mdr->take_w_condition(data, info, 3, dr_rc);
              if (ret != RETCODE_OK && ret != RETCODE_NO_DATA)
                {
                  cout << "ERROR: take_w_condition returned " << ret << endl;
                  passed = false;
                  done = true;
                }
              if (ret == RETCODE_OK)
                {
                  received_data(data, mdw, msg);
                }
              cout << "testing take_next_instance_w_condition" << endl;
              InstanceHandle_t handle = HANDLE_NIL;
              while (true)
                {
                  ret = mdr->take_next_instance_w_condition(data, info, 1,
                                                            handle, dr_rc);
                  if (ret == RETCODE_NO_DATA) break;
                  if (ret != RETCODE_OK)
                    {
                      cout << "ERROR: take_next_instance_w_condition returned "
                           << ret << endl;
                      passed = false;
                      done = true;
                    }
                  received_data(data, mdw, msg);
                  handle = info[info.length() - 1].instance_handle;
                }
            }
          else if (active[i] == dr_rc2)
            {
              cout << "an instance has been disposed, exiting" << endl;
              done = true;
            }
        }
    }
  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);
  ws->detach_condition(dr_rc2);
  dr->delete_readcondition(dr_rc2);

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
