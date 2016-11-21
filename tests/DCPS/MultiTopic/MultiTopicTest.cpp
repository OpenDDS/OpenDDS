#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include "dds/DCPS/StaticIncludes.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "MultiTopicTestTypeSupportImpl.h"

#include <sstream>
#include <stdexcept>

using namespace DDS;
using namespace OpenDDS::DCPS;

const int N_ITERATIONS = 5;

void check(ReturnCode_t ret)
{
  if (ret != RETCODE_OK) {
    std::ostringstream oss;
    oss << "DDS API call failed with return code: " << ret;
    throw std::runtime_error(oss.str());
  }
}

void waitForMatch(const DataWriter_var& dw, int count = 1)
{
  StatusCondition_var sc = dw->get_statuscondition();
  sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(sc);
  const Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  PublicationMatchedStatus pubmatched;
  while (dw->get_publication_matched_status(pubmatched) == RETCODE_OK
         && pubmatched.current_count != count) {
    ws->wait(active, infinite);
  }
  ws->detach_condition(sc);
}

template <typename MessageType>
struct Writer {
  Writer(const Publisher_var& pub, const char* topic_name,
         const DomainParticipant_var& other_participant)
    : ts_(new typename ::OpenDDS::DCPS::DDSTraits<MessageType>::TypeSupportTypeImpl())
  {
    DomainParticipant_var dp = pub->get_participant();
    check(ts_->register_type(dp, ""));
    check(ts_->register_type(other_participant, ""));
    CORBA::String_var type_name = ts_->get_type_name();
    Topic_var topic = dp->create_topic(topic_name, type_name,
      TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    Topic_var topic2 = other_participant->create_topic(topic_name, type_name,
      TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    dw_ = pub->create_datawriter(topic,
      DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  }

  typename OpenDDS::DCPS::DDSTraits<MessageType>::TypeSupportType::_var_type ts_;
  DataWriter_var dw_;
};

bool run_multitopic_test(const Publisher_var& pub, const Subscriber_var& sub)
{
  DomainParticipant_var sub_dp = sub->get_participant();

  // Writer-side setup

  Writer<LocationInfo> location(pub, "Location", sub_dp);
  Writer<PlanInfo> flightplan(pub, "FlightPlan", sub_dp);
  Writer<MoreInfo> more(pub, "More", sub_dp);
  Writer<UnrelatedInfo> unrelated(pub, "Unrelated", sub_dp);
  MoreInfoDataWriter_var midw = MoreInfoDataWriter::_narrow(more.dw_);

  // Reader-side setup

  ResultingTypeSupport_var ts_res = new ResultingTypeSupportImpl;
  check(ts_res->register_type(sub_dp, ""));
  CORBA::String_var type_name = ts_res->get_type_name();
  MoreInfo mi;
  DDS::DataReader_var dr;

  for (int i = 0; i < N_ITERATIONS; ++i) {

    MultiTopic_var mt = sub_dp->create_multitopic("MyMultiTopic", type_name,
      "SELECT flight_name, x, y, z AS height, more, misc "
      "FROM Location NATURAL JOIN FlightPlan NATURAL JOIN More NATURAL JOIN "
      "Unrelated WHERE height < 1000 AND x<23", StringSeq());
    if (!mt) return false;
    dr = sub->create_datareader(mt, DATAREADER_QOS_DEFAULT,
                                0, DEFAULT_STATUS_MASK);

    // Write samples (Location)

    waitForMatch(location.dw_);
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

    waitForMatch(flightplan.dw_);
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

    waitForMatch(more.dw_);
    mi.flight_id1 = 12345;
    mi.more = "Shouldn't see this";
    ret = midw->write(mi, HANDLE_NIL);
    if (ret != RETCODE_OK) return false;
    mi.flight_id1 = 100;
    mi.more = "Extra info for all flights with id1 == 100";
    ret = midw->write(mi, HANDLE_NIL);
    if (ret != RETCODE_OK) return false;

    // Write samples (Unrelated)

    waitForMatch(unrelated.dw_);
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

    // Reader cleanup
    if (i != N_ITERATIONS - 1) {
      sub->delete_datareader(dr);
      waitForMatch(location.dw_, 0);
      waitForMatch(flightplan.dw_, 0);
      waitForMatch(more.dw_, 0);
      waitForMatch(unrelated.dw_, 0);
      sub_dp->delete_multitopic(mt);
    }
  }

  // Dispose

  ReturnCode_t ret = midw->dispose(mi, HANDLE_NIL);
  if (ret != RETCODE_OK) return false;
  ReadCondition_var rc =
    dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE,
                             NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(rc);
  const Duration_t infinite = {DURATION_INFINITE_SEC, DURATION_INFINITE_NSEC};
  ConditionSeq active;
  ret = ws->wait(active, infinite);
  if (ret != RETCODE_OK) return false;
  ws->detach_condition(rc);
  ResultingDataReader_var res_dr = ResultingDataReader::_narrow(dr);
  ResultingSeq data;
  SampleInfoSeq info;
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

  DomainParticipant_var dp2 =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);

  Subscriber_var sub = dp2->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                              DEFAULT_STATUS_MASK);

  TransportRegistry& treg = *TheTransportRegistry;
  if (!treg.get_config("t1").is_nil()) {
    treg.bind_config("t1", pub);
    treg.bind_config("t2", sub);
  }

  const bool passed = run_multitopic_test(pub, sub);

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  dp2->delete_contained_entities();
  dpf->delete_participant(dp2);
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
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
