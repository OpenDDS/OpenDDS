#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <MultiTopicTestTypeSupportImpl.h>

#include <sstream>
#include <stdexcept>

using namespace DDS;
using namespace OpenDDS::DCPS;

/*
 * These wrappers make it possible to use this source code with both the
 * classic C++ mapping and the C++11 mapping
 */
#ifdef CLASSIC_MAPPING
bool operator!=(const TAO::String_Manager& a, const TAO::String_Manager& b)
{
  return std::strcmp(a.in(), b.in());
}

bool operator!=(const Airports::element_type& a, const Airports::element_type& b)
{
  return std::strcmp(a.in(), b.in());
}

template <typename T>
size_t get_sequence_length(T& seq)
{
  return seq.length();
}

template <typename T>
void set_sequence_length(T& seq, size_t length)
{
  seq.length(length);
}

template <typename T>
bool sequences_are_not_equal(const T& a, const T& b)
{
  const unsigned len = a.length();
  if (len != b.length()) {
    return true;
  }
  for (unsigned i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return true;
    }
  }
  return false;
}

template <>
bool sequences_are_not_equal<Airports>(const Airports& a, const Airports& b)
{
  const unsigned len = a.length();
  if (len != b.length()) {
    return true;
  }
  for (unsigned i = 0; i < len; i++) {
    if (strcmp(a[i], b[i])) {
      return true;
    }
  }
  return false;
}

class LocationInfoWrapper {
public:
  CORBA::ULong& flight_id() { return value_.flight_id; }
  CORBA::ULong& departure_date() { return value_.departure_date; }
  CORBA::Long& x() { return value_.x; }
  CORBA::Long& y() { return value_.y; }
  CORBA::Long& z() { return value_.z; }

  operator LocationInfo() const { return value_; }

private:
  LocationInfo value_;
};

class PlanInfoWrapper {
public:
  CORBA::ULong& flight_id() { return value_.flight_id; }
  CORBA::ULong& departure_date() { return value_.departure_date; }
  TAO::String_Manager& flight_name() { return value_.flight_name; }
  TAO::String_Manager& tailno() { return value_.tailno; }
  FlightType& type() { return value_.type; }
  TAO::String_Manager& departure() { return value_.departure; }
  TAO::String_Manager& destination() { return value_.destination; }
  Airports& alternative_destinations() { return value_.alternative_destinations; }

  operator PlanInfo() const { return value_; }

private:
  PlanInfo value_;
};

class MoreInfoWrapper {
public:
  CORBA::ULong& departure_date() { return value_.departure_date; }
  TAO::String_Manager& more() { return value_.more; }
  EvenMore& even_more() { return value_.even_more; }

  operator MoreInfo() const { return value_; }

private:
  MoreInfo value_;
};

class UnrelatedInfoWrapper {
public:
  TAO::String_Manager& misc() { return value_.misc; }

  operator UnrelatedInfo() const { return value_; }

private:
  UnrelatedInfo value_;
};

class ResultingWrapper {
public:
  ResultingWrapper(const Resulting& value)
  : value_(value)
  {
  }

  CORBA::ULong& flight_id() { return value_.flight_id; }
  CORBA::ULong& departure_date() { return value_.departure_date; }
  TAO::String_Manager& flight_name() { return value_.flight_name; }
  FlightType& type() { return value_.type; }
  TAO::String_Manager& destination() { return value_.destination; }
  Airports& alternative_destinations() { return value_.alternative_destinations; }
  CORBA::Long& x() { return value_.x; }
  CORBA::Long& y() { return value_.y; }
  CORBA::Long& height() { return value_.height; }
  TAO::String_Manager& more() { return value_.more; }
  EvenMore& even_more() { return value_.even_more; }
  TAO::String_Manager& misc() { return value_.misc; }

  operator Resulting() const { return value_; }

private:
  Resulting value_;
};

#elif defined(CPP11_MAPPING)
template <typename T>
size_t get_sequence_length(T& seq)
{
  return seq.size();
}

template <typename T>
void set_sequence_length(std::vector<T>& seq, size_t length)
{
  seq.resize(length);
}

template <typename T>
bool sequences_are_not_equal(const T& a, const T& b)
{
  return a != b;
}

std::ostream& operator<<(std::ostream& os, FlightType value)
{
  switch (value) {
  case FlightType::visual:
    os << "visual";
    return os;
  case FlightType::instrument:
    os << "instrument";
    return os;
  case FlightType::mixed:
    os << "mixed";
    return os;
  }
  throw std::runtime_error("Invalid value passed to operation<<(std::ostream, FlightType)");
}

using LocationInfoWrapper = LocationInfo;
using PlanInfoWrapper = PlanInfo;
using MoreInfoWrapper = MoreInfo;
using UnrelatedInfoWrapper = UnrelatedInfo;
using ResultingWrapper = Resulting;
#endif

template <typename T>
bool arrays_are_not_equal(const T* a, const T* b, size_t length)
{
  for (size_t i = 0; i < length; i++) {
    if (a[i] != b[i]) {
      return true;
    }
  }
  return false;
}

template <typename T>
void print_array(std::ostream& os, const T* array, size_t length)
{
  for (size_t i = 0; i < length; i++) {
    if (i) {
      os << ", ";
    }
    os << array[i];
  }
}

template <typename T>
void print_sequence(std::ostream& os, const T& seq) {
  for (size_t i = 0; i < get_sequence_length(seq); i++) {
    if (i) {
      os << ", ";
    }
    os << seq[i];
  }
}

template <typename T>
bool expect(const char* what, const T& expected, const T& result)
{
  if (expected != result) {
    std::cerr
      << "ERROR: Expected " << what << " to be (" << expected
      << "), but it was (" << result << ')' << std::endl;
    return true;
  }
  return false;
}

template <typename T>
bool expect_array(const char* what, const T* expected, const T* result, size_t length)
{
  if (arrays_are_not_equal(expected, result, length)) {
    std::cerr << "ERROR: Expected " << what << " to be {";
    print_array(std::cerr, expected, length);
    std::cerr << "}, but it was {";
    print_array(std::cerr, result, length);
    std::cerr << '}' << std::endl;
    return true;
  }
  return false;
}

template <typename T>
bool expect_sequence(const char* what, const T& expected, const T& result)
{
  if (sequences_are_not_equal(expected, result)) {
    std::cerr << "ERROR: Expected " << what << " to be {";
    print_sequence(std::cerr, expected);
    std::cerr << "}, but it was {";
    print_sequence(std::cerr, result);
    std::cerr << '}' << std::endl;
    return true;
  }
  return false;
}

const int N_ITERATIONS = 5;
const Duration_t max_wait = {10, 0};

void check_rc(ReturnCode_t ret, const char* message, const char* extra_message = 0)
{
  if (ret != RETCODE_OK) {
    std::ostringstream oss;
    oss << "Failed to " << message;
    if (extra_message) {
      oss << ' ' << extra_message;
    }
    oss << ": " << retcode_to_string(ret);
    throw std::runtime_error(oss.str());
  }
}

void waitForMatch(const DataWriter_var& dw, int count = 1)
{
  StatusCondition_var sc = dw->get_statuscondition();
  sc->set_enabled_statuses(PUBLICATION_MATCHED_STATUS);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(sc);
  ConditionSeq active;
  PublicationMatchedStatus pubmatched;
  while (dw->get_publication_matched_status(pubmatched) == RETCODE_OK
         && pubmatched.current_count != count) {
    check_rc(ws->wait(active, max_wait), "wait for match");
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
    check_rc(ts_->register_type(dp, ""), "register type", DDSTraits<MessageType>::type_name());
    check_rc(ts_->register_type(other_participant, ""), "register type", DDSTraits<MessageType>::type_name());
    CORBA::String_var type_name = ts_->get_type_name();
    Topic_var topic = dp->create_topic(topic_name, type_name,
      TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

    TopicQos topic_qos;
    other_participant->get_default_topic_qos(topic_qos);
    topic_qos.latency_budget.duration.sec = 1;
    topic_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    Topic_var topic2 = other_participant->create_topic(topic_name, type_name,
      topic_qos, 0, DEFAULT_STATUS_MASK);

    dw_ = pub->create_datawriter(topic,
      DATAWRITER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  }

  typename OpenDDS::DCPS::DDSTraits<MessageType>::TypeSupportType::_var_type ts_;
  DataWriter_var dw_;
};

bool check_bits(const Publisher_var& pub)
{
#ifdef DDS_HAS_MINIMUM_BIT
  ACE_UNUSED_ARG(pub);
  return true;
#else
  DomainParticipant_var pub_dp = pub->get_participant();
  Subscriber_var bit_sub = pub_dp->get_builtin_subscriber();
  DataReader_var bit_dr = bit_sub->lookup_datareader(BUILT_IN_SUBSCRIPTION_TOPIC);
  SubscriptionBuiltinTopicDataDataReader_var dr =
    SubscriptionBuiltinTopicDataDataReader::_narrow(bit_dr);
  SubscriptionBuiltinTopicDataSeq data;
  SampleInfoSeq info;
  check_rc(dr->read(data, info, LENGTH_UNLIMITED, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE),
    "read builtin subscription topic");
  for (unsigned int i = 0; i < data.length(); ++i) {
    if (std::strcmp(data[i].topic_name, "Location") == 0) {
      if (data[i].latency_budget.duration.sec != 1) {
        std::cerr << "ERROR: Built-In DataReader Location topic unexpected QoS\n";
        return false;
      }
      return true;
    }
  }
  std::cerr << "ERROR: Built-In DataReader Location topic not found\n";
  return false;
#endif
}

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
  check_rc(ts_res->register_type(sub_dp, ""), "register resulting type");
  CORBA::String_var type_name = ts_res->get_type_name();
  MoreInfoWrapper mi;
  DDS::DataReader_var dr;

  for (int i = 0; i < N_ITERATIONS; ++i) {

    MultiTopic_var mt = sub_dp->create_multitopic("MyMultiTopic", type_name,
      "SELECT flight_name, type, destination, alternative_destinations, x, y, z AS height, more, even_more, misc "
      "FROM Location NATURAL JOIN FlightPlan NATURAL JOIN More NATURAL JOIN Unrelated "
      "WHERE height < 1000 AND x < 23 AND type = 'instrument'", StringSeq());
    if (!mt) {
      throw std::runtime_error("failed to create multitopic");
    }
    dr = sub->create_datareader(mt, DATAREADER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

    // Write samples (Location)
    waitForMatch(location.dw_);
    LocationInfoDataWriter_var locdw = LocationInfoDataWriter::_narrow(location.dw_);

    LocationInfoWrapper loc97;
    loc97.flight_id() = 97;
    loc97.departure_date() = 103;
    loc97.x() = 23;
    loc97.y() = 2;
    loc97.z() = 3;
    check_rc(locdw->write(loc97, HANDLE_NIL), "write loc97"); // filtered out (x < 23)

    LocationInfoWrapper loc96;
    loc96.flight_id() = 96;
    loc96.departure_date() = 103;
    loc96.x() = 1;
    loc96.y() = 2;
    loc96.z() = 3000;
    check_rc(locdw->write(loc96, HANDLE_NIL), "write loc96"); // filtered out (height < 1000)

    LocationInfoWrapper loc99;
    loc99.flight_id() = 99;
    loc99.departure_date() = 103;
    loc99.x() = 1;
    loc99.y() = 2;
    loc99.z() = 3;
    check_rc(locdw->write(loc99, HANDLE_NIL), "write loc99"); // matches plan99, isn't filtered out

    LocationInfoWrapper loc98;
    loc98.flight_id() = 98;
    loc98.departure_date() = 102;
    loc98.x() = 4;
    loc98.y() = 5;
    loc98.z() = 6;
    check_rc(locdw->write(loc98, HANDLE_NIL), "write loc98"); // filtered out, no PlanInfo

    // Write samples (FlightPlan)
    waitForMatch(flightplan.dw_);
    PlanInfoDataWriter_var pidw = PlanInfoDataWriter::_narrow(flightplan.dw_);

    PlanInfoWrapper plan99;
    plan99.flight_id() = 99;
    plan99.departure_date() = 103;
    plan99.flight_name() = "Flight 100-99";
    plan99.type() = FlightType::instrument;
    plan99.tailno() = "N12345";
    plan99.departure() = "STL";
    plan99.destination() = "ALN";
    set_sequence_length(plan99.alternative_destinations(), 2);
    plan99.alternative_destinations()[0] = "NHK";
    plan99.alternative_destinations()[1] = "TXL";
    check_rc(pidw->write(plan99, HANDLE_NIL), "write plan99");

    PlanInfoWrapper plan97(plan99);
    plan97.flight_id() = 97;
    plan97.flight_name() = "Flight 100-97";
    plan97.departure() = "STL";
    plan97.destination() = "LAX";
    set_sequence_length(plan97.alternative_destinations(), 3);
    plan97.alternative_destinations()[2] = "BLV";
    check_rc(pidw->write(plan97, HANDLE_NIL), "write plan97");

    PlanInfoWrapper plan96(plan99);
    plan96.flight_id() = 96;
    plan96.flight_name() = "Flight 100-96";
    plan96.type() = FlightType::mixed;
    plan96.departure() = "BER";
    plan96.destination() = "LHR";
    set_sequence_length(plan96.alternative_destinations(), 3);
    plan96.alternative_destinations()[2] = "CDG";
    check_rc(pidw->write(plan96, HANDLE_NIL), "write plan96");

    // Write samples (More)
    waitForMatch(more.dw_);

    mi.departure_date() = 299;
    mi.more() = "Shouldn't see this";
    for (unsigned i = 0; i < even_more_length; i++) {
      mi.even_more()[i] = i;
    }
    check_rc(midw->write(mi, HANDLE_NIL), "write first mi");

    mi.departure_date() = 103;
    mi.more() = "Extra info for all flights departing on 103";
    for (unsigned i = 0; i < even_more_length; i++) {
      mi.even_more()[i] = i + 1;
    }
    check_rc(midw->write(mi, HANDLE_NIL), "write second mi");

    // Write samples (Unrelated)
    waitForMatch(unrelated.dw_);
    UnrelatedInfoDataWriter_var uidw = UnrelatedInfoDataWriter::_narrow(unrelated.dw_);
    UnrelatedInfoWrapper ui;
    ui.misc() = "Misc";
    check_rc(uidw->write(ui, HANDLE_NIL), "write ui");

    // Read resulting samples
    WaitSet_var ws = new WaitSet;
    ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
      ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ws->attach_condition(rc);
    ConditionSeq active;
    check_rc(ws->wait(active, max_wait), "wait for samples");
    ws->detach_condition(rc);
    ResultingDataReader_var res_dr = ResultingDataReader::_narrow(dr);
    ResultingSeq data;
    SampleInfoSeq info;
    check_rc(res_dr->take_w_condition(data, info, LENGTH_UNLIMITED, rc), "take");
    if (data.length() > 1) {
      std::cerr << "ERROR: take got more than expected" << std::endl;
      return false;
    }
    if (!info[0].valid_data) {
      std::cerr << "ERROR: expected take to return valid data" << std::endl;
      return false;
    }

    // Print the Result
    ResultingWrapper rw(data[0]);
    std::cout << "Received: "
      << rw.flight_id() << " on " << rw.departure_date() << " \"" << rw.flight_name() << "\" "
      << (rw.type() == FlightType::instrument ? "instrument" : "INVALID TYPE") << ' '
      << rw.destination() << ", alternatively {";
    print_sequence(std::cout, rw.alternative_destinations());
    std::cout
      << "} " << rw.x() << " " << rw.y() << " " << rw.height()
      << " \"" << rw.more() << "\" { ";
    print_array(std::cout, &rw.even_more()[0], even_more_length);
    std::cout << "} \"" << rw.misc() << "\"" << std::endl;

    // Check the Result
    bool invalid_result = false;
    invalid_result |= expect("flight_id", rw.flight_id(), plan99.flight_id());
    invalid_result |= expect("departure_date", rw.departure_date(), plan99.departure_date());
    invalid_result |= expect("flight_name", rw.flight_name(), plan99.flight_name());
    invalid_result |= expect("type", rw.type(), plan99.type());
    invalid_result |= expect("destination", rw.destination(), plan99.destination());
    invalid_result |= expect_sequence(
      "alternative_destinations", rw.alternative_destinations(), plan99.alternative_destinations());
    invalid_result |= expect("x", rw.x(), loc99.x());
    invalid_result |= expect("y", rw.y(), loc99.y());
    invalid_result |= expect("height", rw.height(), loc99.z());
    invalid_result |= expect("more", rw.more(), mi.more());
    invalid_result |= expect_array("even_more", &rw.even_more()[0], &mi.even_more()[0], even_more_length);
    invalid_result |= expect("misc", rw.misc(), ui.misc());
    if (invalid_result) {
      return false;
    }

    // Check return get_key_value
    // Regression Test for https://github.com/objectcomputing/OpenDDS/issues/592
    {
      Resulting resulting_value;
      ReturnCode_t rc = res_dr->get_key_value(resulting_value, HANDLE_NIL);
      if (rc != RETCODE_BAD_PARAMETER) {
        std::ostringstream oss;
        oss
          << "Expected get_key_value for HANDLE_NIL to return bad param, but it returned "
          << retcode_to_string(rc);
        throw std::runtime_error(oss.str());
      }
    }

    // Make Sure Theres No Data Left
    data.length(0);
    info.length(0);
    {
      ReturnCode_t ret = res_dr->read_w_condition(data, info, LENGTH_UNLIMITED, rc);
      if (ret != RETCODE_NO_DATA) {
        std::ostringstream oss;
        oss
          << "Expected read_w_condition to return no data, but it returned "
          << retcode_to_string(ret);
        throw std::runtime_error(oss.str());
      }
    }
    dr->delete_readcondition(rc);

    if (!check_bits(pub)) {
      return false;
    }

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
  check_rc(midw->dispose(mi, HANDLE_NIL), "dispose mi");
  ReadCondition_var rc =
    dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE,
                             NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(rc);
  ConditionSeq active;
  check_rc(ws->wait(active, max_wait), "wait for dispose");
  ws->detach_condition(rc);
  ResultingDataReader_var res_dr = ResultingDataReader::_narrow(dr);
  ResultingSeq data;
  SampleInfoSeq info;
  check_rc(res_dr->read_w_condition(data, info, LENGTH_UNLIMITED, rc), "");
  dr->delete_readcondition(rc);

  return !(info[0].valid_data || info[0].instance_state != NOT_ALIVE_DISPOSED_INSTANCE_STATE);
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
    std::cerr << "ERROR: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "ERROR: unknown exception in main" << std::endl;
  }
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
