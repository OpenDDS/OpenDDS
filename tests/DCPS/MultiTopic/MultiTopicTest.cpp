#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#include "MultiTopicTestTypeSupportImpl.h"

using namespace DDS;
using namespace OpenDDS::DCPS;

void waitForMatch(const StatusCondition_var& sc)
{
  sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(sc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ws->wait(active, infinite);
  ws->detach_condition(sc);
}

bool run_multitopic_test(const DomainParticipant_var& dp,
  const ResultingTypeSupport_var& ts, const Publisher_var& pub,
  const Subscriber_var& sub)
{
  // Writer-side setup

  LocationInfoTypeSupport_var ts_loc = new LocationInfoTypeSupportImpl;
  ts_loc->register_type(dp, "");
  CORBA::String_var type_name_loc = ts_loc->get_type_name();
  Topic_var location = dp->create_topic("Location", type_name_loc,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  DataWriter_var dw_loc = pub->create_datawriter(location,
    DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  PlanInfoTypeSupport_var ts_pi = new PlanInfoTypeSupportImpl;
  ts_pi->register_type(dp, "");
  CORBA::String_var type_name_pi = ts_pi->get_type_name();
  Topic_var flightplan = dp->create_topic("FlightPlan", type_name_pi,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  DataWriter_var dw_fp = pub->create_datawriter(flightplan,
    DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  // Reader-side setup

  CORBA::String_var type_name = ts->get_type_name();
  MultiTopic_var mt = dp->create_multitopic("MyMultiTopic", type_name,
    "SELECT flight_name, x, y, z AS height FROM Location NATURAL JOIN "
    "FlightPlan WHERE height < 1000 AND x<23", StringSeq());
  if (!mt) return false;
  DataReader_var dr = sub->create_datareader(mt, DATAREADER_QOS_DEFAULT,
    0, DEFAULT_STATUS_MASK);

  // Write samples (Location)

  StatusCondition_var dw_sc = dw_loc->get_statuscondition();
  waitForMatch(dw_sc);
  LocationInfoDataWriter_var locdw = LocationInfoDataWriter::_narrow(dw_loc);
  LocationInfo sample = {100, 23, 2, 3}; // filtered out (x < 23)
  ReturnCode_t ret = locdw->write(sample, HANDLE_NIL);
  LocationInfo sample2 = {100, 1, 2, 3000}; // filtered out (height < 1000)
  ret = locdw->write(sample2, HANDLE_NIL);
  LocationInfo sample3 = {100, 1, 2, 3};
  ret = locdw->write(sample3, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;

  // Write samples (FlightPlan)

  StatusCondition_var dw2_sc = dw_fp->get_statuscondition();
  waitForMatch(dw2_sc);
  PlanInfoDataWriter_var pidw = PlanInfoDataWriter::_narrow(dw_fp);
  PlanInfo sample4;
  sample4.flight_id = 100;
  sample4.flight_name = "Flight 100";
  sample4.tailno = "N12345";
  ret = pidw->write(sample4, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;

  // Read resulting samples

  WaitSet_var ws = new WaitSet;
  ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
    ANY_VIEW_STATE, ANY_INSTANCE_STATE);
  ws->attach_condition(rc);
  Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ret = ws->wait(active, infinite);
  if (ret != RETCODE_OK) return false;
  ws->detach_condition(rc);
  ResultingDataReader_var res_dr = ResultingDataReader::_narrow(dr);
  ResultingSeq data;
  SampleInfoSeq info;
  ret = res_dr->take_w_condition(data, info, DDS::LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) return false;
  if (data.length() > 1 || !info[0].valid_data) return false;
  std::cout << "Received: " << data[0].flight_id << " \""
    << data[0].flight_name << "\" " << data[0].x << " " << data[0].y << " "
    << data[0].height << std::endl;
  if (data[0].flight_id != sample4.flight_id ||
    strcmp(data[0].flight_name, sample4.flight_name) || data[0].x != sample3.x
    || data[0].y != sample3.y || data[0].height != sample3.z) return false;
  data.length(0);
  info.length(0);
  ret = res_dr->read_w_condition(data, info, DDS::LENGTH_UNLIMITED, rc);
  dr->delete_readcondition(rc);
  if (ret != RETCODE_NO_DATA) return false;  
  return true;
}

int run_test(int argc, ACE_TCHAR *argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);
  ResultingTypeSupport_var ts = new ResultingTypeSupportImpl;
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

  bool passed = run_multitopic_test(dp, ts, pub, sub);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  return passed ? 0 : 1;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = 1;
  try {
    ret = run_test(argc, argv);
  } catch (const CORBA::SystemException& e) {
    e._tao_print_exception("ERROR: ");
  } catch (const std::exception& e) {
    std::cout << "ERROR: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "ERROR: unknown exception in main" << std::endl;
  }
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
