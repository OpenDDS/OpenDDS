#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/StaticIncludes.h"

#include "MessengerTypeSupportImpl.h"

#include "ace/Task.h"

class Waiter : public ACE_Task_Base
{
public:
  explicit Waiter(DDS::WaitSet_ptr ws)
    : ws_(DDS::WaitSet::_duplicate(ws))
  {}

  int svc()
  {
    DDS::Duration_t three = {3, 0};
    DDS::ConditionSeq active;
    result_ = ws_->wait(active, three);
    return 0;
  }

  int result() { return result_; }

private:
  DDS::WaitSet_var ws_;
  int result_;
};


int run_test(int argc, ACE_TCHAR *argv[])
{
  using namespace DDS;
  using namespace OpenDDS::DCPS;
  WaitSet_var ws = new WaitSet;
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0, ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  Messenger::MessageTypeSupport_var ts = new Messenger::MessageTypeSupportImpl;
  CORBA::String_var message_type_name = ts->get_type_name();
  ts->register_type(dp, message_type_name);
  Topic_var topic = dp->create_topic("MyTopic", message_type_name,
    TOPIC_QOS_DEFAULT, 0, ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  StatusCondition_var cond = dw->get_statuscondition();
  bool passed = cond->get_enabled_statuses() == ::OpenDDS::DCPS::DEFAULT_STATUS_MASK;
  cond->set_enabled_statuses(OFFERED_INCOMPATIBLE_QOS_STATUS);
  passed = cond->get_enabled_statuses() == OFFERED_INCOMPATIBLE_QOS_STATUS;
  ws->attach_condition(cond);

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.durability.kind = PERSISTENT_DURABILITY_QOS;
  Waiter w(ws);
  w.activate();
  DataReader_var dr = sub->create_datareader(topic, dr_qos, 0,
                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  w.wait();
  passed &= (w.result() == RETCODE_OK);
  ws->detach_condition(cond);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  return passed ? 0 : 1;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);
  TheServiceParticipant->shutdown();
  return ret;
}
