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
using namespace DDS;
using namespace OpenDDS::DCPS;
using namespace Messenger;

bool waitForSample(const DataReader_var& dr)
{
  ReadCondition_var dr_rc = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_rc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ReturnCode_t ret = ws->wait(active, infinite);
  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);
  if (ret != RETCODE_OK) {
    cout << "ERROR: wait(rc) failed" << endl;
    return false;
  }
  return true;
}

bool run_filtering_test(const DomainParticipant_var& dp,
  const MessageTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("MyTopic", type_name,
                                     TOPIC_QOS_DEFAULT, 0,
                                     DEFAULT_STATUS_MASK);

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  DataWriter_var dw =
    pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = KEEP_ALL_HISTORY_QOS;
  ContentFilteredTopic_var cft = dp->create_contentfilteredtopic(
    "MyTopic-Filtered", topic, "key > 1", StringSeq());
  DataReader_var dr =
    sub->create_datareader(cft, dr_qos, 0, DEFAULT_STATUS_MASK);

  StatusCondition_var dw_sc = dw->get_statuscondition();
  dw_sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dw_sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(dw_sc);

  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  Message sample;
  sample.key = 1;
  ReturnCode_t ret = mdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  sample.key = 2;
  if (mdw->write(sample, HANDLE_NIL) != RETCODE_OK) return false;
  if (!waitForSample(dr)) return false;

  {
    MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
    MessageSeq data;
    SampleInfoSeq infoseq;
    ret = mdr->take(data, infoseq, LENGTH_UNLIMITED, ANY_SAMPLE_STATE,
      ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    if (ret != RETCODE_OK) {
      cout << "ERROR: take() should have returned some data" << endl;
      return false;
    }
    size_t count(0);
    for (CORBA::ULong i(0); i < data.length(); ++i) {
      if (infoseq[i].valid_data) {
        ++count;
        cout << "received data with key == " << data[i].key << endl;
      }
    }
    if (count != 1) {
      cout << "ERROR: take() should have returned only one valid sample"
        << endl;
      return false;
    }
  }

  if (dp->delete_contentfilteredtopic(cft) != RETCODE_PRECONDITION_NOT_MET) {
    cout << "ERROR: delete_contentfilteredtopic should return "
      "RETCODE_PRECONDITION_NOT_MET when datareader still exists" << endl;
    return false;
  }

  if (sub->delete_datareader(dr) != RETCODE_OK) {
    cout << "ERROR: delete_datareader" << endl;
    return false;
  }
  dr = DataReader::_nil();

  if (dp->delete_contentfilteredtopic(cft) != RETCODE_OK) {
    cout << "ERROR: delete_contentfilteredtopic" << endl;
    return false;
  }
  return true;
}


int run_test(int argc, ACE_TCHAR *argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);
  MessageTypeSupport_var ts = new MessageTypeSupportImpl;
  ts->register_type(dp, "");

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                           DEFAULT_STATUS_MASK);
  TransportImpl_rch pub_tport =
    TheTransportFactory->create_transport_impl(1, AUTO_CONFIG);
  PublisherImpl* pub_impl = dynamic_cast<PublisherImpl*>(pub.in());
  pub_impl->attach_transport(pub_tport.in());

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
  TransportImpl_rch sub_tport =
    TheTransportFactory->create_transport_impl(2, AUTO_CONFIG);
  SubscriberImpl* sub_impl = dynamic_cast<SubscriberImpl*>(sub.in());
  sub_impl->attach_transport(sub_tport.in());

  bool passed = run_filtering_test(dp, ts, pub, sub);

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
