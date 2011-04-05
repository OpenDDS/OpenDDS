#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

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

template <typename TSImpl>
struct Writer {
  Writer(const DomainParticipant_var& dp, const Publisher_var& pub,
    const char* topic_name)
    : ts_(new TSImpl)
  {
    ts_->register_type(dp, "");
    CORBA::String_var type_name = ts_->get_type_name();
    Topic_var topic = dp->create_topic(topic_name, type_name,
      TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    dw_ = pub->create_datawriter(topic,
      DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  }

  typename TSImpl::_var_type ts_;
  DataWriter_var dw_;
};

bool run_multitopic_test(const DomainParticipant_var& dp,
  const Publisher_var& pub, const Subscriber_var& sub)
{
  // Writer-side setup

  Writer<LocationInfoTypeSupportImpl> location(dp, pub, "Location");
  Writer<PlanInfoTypeSupportImpl> flightplan(dp, pub, "FlightPlan");
  Writer<MoreInfoTypeSupportImpl> more(dp, pub, "More");
  Writer<UnrelatedInfoTypeSupportImpl> unrelated(dp, pub, "Unrelated");

  // Reader-side setup

  ResultingTypeSupport_var ts_res = new ResultingTypeSupportImpl;
  ts_res->register_type(dp, "");
  CORBA::String_var type_name = ts_res->get_type_name();
  MultiTopic_var mt = dp->create_multitopic("MyMultiTopic", type_name,
    "SELECT flight_name, x, y, z AS height, more, misc "
    "FROM Location NATURAL JOIN FlightPlan NATURAL JOIN More NATURAL JOIN "
    "Unrelated WHERE height < 1000 AND x<23", StringSeq());
  if (!mt) return false;
  DataReader_var dr = sub->create_datareader(mt, DATAREADER_QOS_DEFAULT,
    0, DEFAULT_STATUS_MASK);

  // Write samples (Location)

  StatusCondition_var dw_sc = location.dw_->get_statuscondition();
  waitForMatch(dw_sc);
  LocationInfoDataWriter_var locdw =
    LocationInfoDataWriter::_narrow(location.dw_);
  LocationInfo sample = {100, 97, 23, 2, 3}; // filtered out (x < 23)
  ReturnCode_t ret = locdw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  LocationInfo sample2 = {100, 96, 1, 2, 3000}; // filtered out (height < 1000)
  ret = locdw->write(sample2, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  LocationInfo sample3 = {100, 99, 1, 2, 3};
  ret = locdw->write(sample3, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  LocationInfo sample3_5 = {100, 98, 4, 5, 6};
  ret = locdw->write(sample3_5, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;

  // Write samples (FlightPlan)

  StatusCondition_var dw2_sc = flightplan.dw_->get_statuscondition();
  waitForMatch(dw2_sc);
  PlanInfoDataWriter_var pidw = PlanInfoDataWriter::_narrow(flightplan.dw_);
  PlanInfo sample4;
  sample4.flight_id1 = 100;
  sample4.flight_id2 = 99;
  sample4.flight_name = "Flight 100-99";
  sample4.tailno = "N12345";
  ret = pidw->write(sample4, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  PlanInfo sample4_1(sample4);
  sample4_1.flight_id2 = 97;
  sample4_1.flight_name = "Flight 100-97";
  ret = pidw->write(sample4_1, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  PlanInfo sample4_2(sample4);
  sample4_2.flight_id2 = 96;
  sample4_2.flight_name = "Flight 100-96";
  ret = pidw->write(sample4_2, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;

  // Write samples (More)

  StatusCondition_var dw3_sc = more.dw_->get_statuscondition();
  waitForMatch(dw3_sc);
  MoreInfoDataWriter_var midw = MoreInfoDataWriter::_narrow(more.dw_);
  MoreInfo mi;
  mi.flight_id1 = 12345;
  mi.more = "Shouldn't see this";
  ret = midw->write(mi, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  mi.flight_id1 = 100;
  mi.more = "Extra info for all flights with id1 == 100";
  ret = midw->write(mi, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;

  // Write samples (Unrelated)

  StatusCondition_var dw4_sc = unrelated.dw_->get_statuscondition();
  waitForMatch(dw4_sc);
  UnrelatedInfoDataWriter_var uidw =
    UnrelatedInfoDataWriter::_narrow(unrelated.dw_);
  UnrelatedInfo ui;
  ui.misc = "Misc";
  ret = uidw->write(ui, HANDLE_NIL);
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
  ret = res_dr->take_w_condition(data, info, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) return false;
  if (data.length() > 1 || !info[0].valid_data) return false;
  std::cout << "Received: " << data[0].flight_id1 << '-' <<
    data[0].flight_id2 << " \"" << data[0].flight_name << "\" " << data[0].x <<
    " " << data[0].y << " " << data[0].height << " \"" << data[0].more <<
    "\" \"" << data[0].misc << "\"" << std::endl;
  if (data[0].flight_id1 != sample4.flight_id1 || data[0].flight_id2 !=
      sample4.flight_id2 || strcmp(data[0].flight_name, sample4.flight_name) ||
      data[0].x != sample3.x || data[0].y != sample3.y ||
      data[0].height != sample3.z || strcmp(data[0].more, mi.more) ||
      strcmp(data[0].misc, ui.misc)) {
    return false;
  }
  data.length(0);
  info.length(0);
  ret = res_dr->read_w_condition(data, info, LENGTH_UNLIMITED, rc);
  dr->delete_readcondition(rc);
  if (ret != RETCODE_NO_DATA) return false;

  // Dispose

  ret = midw->dispose(mi, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE,
                                NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  ws->attach_condition(rc);
  active.length(0);
  ret = ws->wait(active, infinite);
  if (ret != RETCODE_OK) return false;
  ws->detach_condition(rc);
  ret = res_dr->read_w_condition(data, info, LENGTH_UNLIMITED, rc);
  dr->delete_readcondition(rc);
  if (ret != RETCODE_OK) return false;
  if (info[0].valid_data ||
      info[0].instance_state != NOT_ALIVE_DISPOSED_INSTANCE_STATE) return false;
  return true;
}

int run_test(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);

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

  bool passed = run_multitopic_test(dp, pub, sub);

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
