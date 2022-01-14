/**
 * This test checks how data representation annotations and QoS work together.
 * The gist of it is that `data_representation` annotations on topic types can
 * be overridden by `representation` topic QoS which can be overridden by
 * `representation` reader and writer QoS.
 *
 * Each test case will create a reader and a writer,
 * check match or flag incompatible QoS, and then destroy the pair.
 * See Test::test_case() and Test::run() for details.
 */
#include "DataRepresentationTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/DCPS_Utils.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  ifdef OPENDDS_SAFETY_PROFILE
#    include <dds/DCPS/StaticDiscovery.h>
#  endif
#endif

#include <vector>
#include <sstream>

using OpenDDS::DCPS::retcode_to_string;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

template<typename Type>
class RegisteredType {
public:
  static const char* name(){ return Traits::type_name(); }
  RegisteredType(DDS::DomainParticipant* p) : type_support_(new TypeSupportImpl), participant_(p) {
    DDS::ReturnCode_t rc = type_support_->register_type(participant_, "");
    if (DDS::RETCODE_OK != rc) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: RegisteredType<%C> failed: %C\n"), name(), retcode_to_string(rc)));
      throw 1;
    }
    ACE_DEBUG((LM_INFO, ACE_TEXT("INFO: register_type[%C]\n"), name()));
  }
  ~RegisteredType() {
    ACE_DEBUG((LM_INFO, ACE_TEXT("INFO: unregister_type[%C]\n"), name()));
    type_support_->unregister_type(participant_, name());
  }
private:
  typedef typename OpenDDS::DCPS::DDSTraits<Type> Traits;
  typedef typename Traits::TypeSupportType::_var_type TypeSupportVar;
  typedef typename Traits::TypeSupportImplType TypeSupportImpl;
  TypeSupportVar type_support_;
  DDS::DomainParticipant* participant_;
};

class Test {
public:
  Test(int argc, ACE_TCHAR** argv);
  ~Test() { cleanup(); }
  int run();

private:
  void cleanup();
  typedef std::vector<DDS::DataRepresentationId_t> Dri;
  void create_topic(const char* type_name, const Dri& dri);
  DDS::DataWriter* create_writer(const Dri& dri = Dri());
  DDS::DataReader* create_reader(const Dri& dri = Dri());

  void test_case(const Dri& writer_dr, const Dri& reader_dr,
                 bool expect_writer = true, bool expect_reader = true, bool expect_match = true);
  void test_default(bool expect_writer = true, bool expect_reader = true);
  void test(bool expect_writer = true, bool expect_reader = true);
  void test_Registered_DefaultType();
  void test_Registered_Xcdr2Xcdr1Type();
  void test_Registered_Xcdr1Type();
  void test_Registered_Xcdr2Type();
  void test_Registered_XmlType();

  static void dr_to_qos(const Dri& dri, DDS::DataRepresentationQosPolicy& qos);
  static std::string to_string(const Dri& dri);
  static bool check_policies(const char* what, bool expect_match, const DDS::QosPolicyCountSeq& q);
  enum CheckMatchResult { check_match_error, check_match_expected, check_match_unexpected };
  CheckMatchResult check_match(DDS::DataReader* reader, DDS::DataWriter* writer, bool expect_match);
  void add_result(bool passed);

  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var participant_;
  DDS::Publisher_var publisher_;
  DDS::Subscriber_var subscriber_;
  std::string topic_name_;
  DDS::Topic_var topic_;
  unsigned cases_passed_;
  unsigned cases_total_;
  const Dri default_dr_;
  Dri xcdr2xcdr1_;
  Dri xcdr1_;
  Dri xcdr2_;
  Dri xml_;
};

Test::Test(int argc, ACE_TCHAR** argv) : cases_passed_(0), cases_total_(0)
{
  try {
    if (!(dpf_ = TheParticipantFactoryWithArgs(argc, argv))) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: Participant Factory failed!\n"))); throw 1;
    }
    if (!(participant_ = dpf_->create_participant(143, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK))) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: create_participant failed!\n"))); throw 1;
    }
    if (!(publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK))) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: create_publisher failed!\n"))); throw 1;
    }
    if (!(subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK))) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: create_subscriber failed!\n"))); throw 1;
    }
    xcdr2xcdr1_.push_back(DDS::XCDR2_DATA_REPRESENTATION);
    xcdr2xcdr1_.push_back(DDS::XCDR_DATA_REPRESENTATION);
    xcdr1_.push_back(DDS::XCDR_DATA_REPRESENTATION);
    xcdr2_.push_back(DDS::XCDR2_DATA_REPRESENTATION);
    xml_.push_back(DDS::XML_DATA_REPRESENTATION);
  } catch (...) { cleanup(); throw; }
}

int Test::run()
{
  test_Registered_DefaultType();
  test_Registered_Xcdr2Xcdr1Type();
  test_Registered_Xcdr1Type();
  test_Registered_Xcdr2Type();
  test_Registered_XmlType();

  const unsigned n_failed = cases_total_ - cases_passed_;
  if (n_failed == 0) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("INFO: %u of %u cases passed\n"), cases_passed_, cases_total_));
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: %u of %u cases failed\n"), n_failed, cases_total_));
  }
  return (n_failed == 0) ? 0 : 1;
}

void Test::cleanup()
{
  if (participant_ && participant_->delete_contained_entities() != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: ERROR: delete_contained_entities failed\n")));
  }
  if (dpf_ && participant_ && dpf_->delete_participant(participant_.in()) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: ERROR: delete_participant failed\n")));
  }
  participant_ = 0;
  dpf_ = 0;
  TheServiceParticipant->shutdown();
}

void Test::create_topic(const char* type_name, const Dri& dri)
{
  topic_name_ = std::string(type_name) + " Topic";
  if (dri.size()) { topic_name_ += " " + to_string(dri); }
  DDS::TopicQos qos;
  if (participant_->get_default_topic_qos(qos) == DDS::RETCODE_OK) {
    dr_to_qos(dri, qos.representation);
    topic_ = participant_->create_topic(topic_name_.c_str(), type_name, qos, 0, DEFAULT_STATUS_MASK);
    ACE_DEBUG((LM_INFO, ACE_TEXT("INFO: create_topic[%C]\n"), topic_name_.c_str()));
  } else { ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: get_default_topic_qos failed!\n"))); }
}

DDS::DataWriter* Test::create_writer(const Dri& dri)
{
  DDS::DataWriterQos qos;
  if (publisher_->get_default_datawriter_qos(qos) == DDS::RETCODE_OK) {
    dr_to_qos(dri, qos.representation);
    return publisher_->create_datawriter(topic_, qos, 0, DEFAULT_STATUS_MASK);
  }
  ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: get_default_datawriter_qos failed!\n")));
  return 0;
}

DDS::DataReader* Test::create_reader(const Dri& dri)
{
  DDS::DataReaderQos qos;
  if (subscriber_->get_default_datareader_qos(qos) == DDS::RETCODE_OK) {
    dr_to_qos(dri, qos.representation);
    return subscriber_->create_datareader(topic_, qos, 0, DEFAULT_STATUS_MASK);
  }
  ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: get_default_datareader_qos failed!\n")));
  return 0;
}

void Test::test_case(const Dri& writer_dr, const Dri& reader_dr,
  bool expect_writer, bool expect_reader, bool expect_match)
{
  if (!topic_) { add_result(false); return; }
  // Create Writer and Reader
  DDS::DataWriter_var writer = create_writer(writer_dr);
  if (!writer) { add_result(!expect_writer); return; }
  DDS::DataReader_var reader = create_reader(reader_dr);
  if (!reader) { add_result(!expect_reader); return; }

  // Check if they matched as expected
  bool passed = false;
  switch (check_match(reader, writer, expect_match)) {
  case check_match_expected: passed = true; break;
  case check_match_unexpected:
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: Test case failed:\n")
      ACE_TEXT("  Topic: \"%C\"\n")
      ACE_TEXT("  Writer Data Representation QoS: %C\n")
      ACE_TEXT("  Reader Data Representation QoS: %C\n")
      ACE_TEXT("  %Cxpected to match.\n"), topic_name_.c_str(),
      to_string(writer_dr).c_str(),
      to_string(reader_dr).c_str(),
      expect_match ? "E" : "NOT e"));
    break;
  case check_match_error: break;
  }

  // Cleanup
  DDS::ReturnCode_t rc = publisher_->delete_datawriter(writer);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: Failed to delete writer: %C\n"),
      retcode_to_string(rc)));
    passed = false;
  }
  rc = subscriber_->delete_datareader(reader);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: Failed to delete reader: %C\n"),
      retcode_to_string(rc)));
    passed = false;
  }
  add_result(passed);
}

void Test::test_default(bool expect_writer, bool expect_reader)
{
  test_case(default_dr_, default_dr_, expect_writer, expect_reader);
  test_case(default_dr_, xcdr2xcdr1_, expect_writer, expect_reader);
  test_case(xcdr2xcdr1_, default_dr_, expect_writer, expect_reader);
  test_case(xcdr2xcdr1_, xcdr2xcdr1_, expect_writer, expect_reader);
}

void Test::test(bool expect_writer, bool expect_reader)
{
  test_default(expect_writer, expect_reader);
  test_case(xcdr2_, default_dr_, expect_writer);
  test_case(default_dr_, xcdr2_, expect_writer, expect_reader);
  test_case(xml_, default_dr_, false, true, false);
  test_case(default_dr_, xml_, expect_writer, false, false);
}

void Test::test_Registered_DefaultType()
{
  RegisteredType<DefaultType> default_type(participant_.in());
  create_topic(default_type.name(), default_dr_);
  test_case(xcdr1_, xcdr1_);
  test_case(xcdr1_, xcdr2xcdr1_);
  test_case(xcdr2xcdr1_, xcdr1_);
  test_case(xcdr2_, xcdr2xcdr1_);
  test_case(xcdr2xcdr1_, xcdr2_);
  test();

  create_topic(default_type.name(), xcdr2_);
  test();

  create_topic(default_type.name(), xml_);
  test();
}

void Test::test_Registered_Xcdr2Xcdr1Type()
{
  RegisteredType<Xcdr2Xcdr1Type> xcdr2xcdr1_type(participant_.in());
  create_topic(xcdr2xcdr1_type.name(), default_dr_);
  test();

  create_topic(xcdr2xcdr1_type.name(), xcdr2_);
  test();
}

void Test::test_Registered_Xcdr1Type()
{
  RegisteredType<Xcdr1Type> xcdr1_type(participant_.in());
  create_topic(xcdr1_type.name(), default_dr_);
  test(false, true);

  create_topic(xcdr1_type.name(), xcdr2_);
  test(false, true);

  create_topic(xcdr1_type.name(), xml_);
  test(false, false);
  test_case(xml_, xml_, false, false);
}

void Test::test_Registered_Xcdr2Type()
{
  RegisteredType<Xcdr2Type> xcdr2_type(participant_.in());
  create_topic(xcdr2_type.name(), default_dr_);
  test();

  create_topic(xcdr2_type.name(), xcdr2_);
  test();
}

void Test::test_Registered_XmlType()
{
  RegisteredType<XmlType> xml_type(participant_.in());
  create_topic(xml_type.name(), default_dr_);
  test_default(false, false);
  test_case(xml_, xml_, false, false);
  test_case(default_dr_, default_dr_, false);
  test_case(xcdr2xcdr1_, default_dr_, false);
  test_case(default_dr_, xcdr2xcdr1_, false);

  test_case(xcdr2_, default_dr_, false, true, false);
  test_case(default_dr_, xcdr2_, false, true, false);
  test_case(xml_, default_dr_, false, false, false);
  test_case(default_dr_, xml_, false, true, false);
}

void Test::dr_to_qos(const Dri& dri, DDS::DataRepresentationQosPolicy& qos)
{
  const Dri::size_type count = dri.size();
  if (count) {
    qos.value.length(static_cast<unsigned>(count));
    for (CORBA::ULong i = 0; i != count; ++i) {
      qos.value[i] = dri[i];
    }
  }
}

std::string Test::to_string(const Dri& dri)
{
  if (dri.size()) {
    std::stringstream ss;
    bool first = true;
    for (Dri::const_iterator i = dri.begin(); i != dri.end(); ++i) {
      if (first) {
        first = false;
      } else {
        ss << ", ";
      }
      if(*i == DDS::XCDR_DATA_REPRESENTATION) {
        ss << "XCDR1";
      } else if (*i == DDS::XCDR2_DATA_REPRESENTATION) {
        ss << "XCDR2";
      } else if (*i == DDS::XML_DATA_REPRESENTATION) {
        ss << "XML";
      } else {
        ss << "Unknown Value " << *i;
      }
    }
    return ss.str();
  }
  return "Default";
}

bool Test::check_policies(const char* what, bool expect_match, const DDS::QosPolicyCountSeq& q)
{
  const CORBA::ULong exp = expect_match ? 0 : 1;
  const CORBA::ULong cnt = q.length();
  bool rv = true;
  bool print = false;
  if (exp != cnt) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: %C %d != %d\n"), what, exp, cnt));
    rv = false;
    print = cnt > 0;
  } else if (cnt == 1 && q[0].policy_id != DDS::DATA_REPRESENTATION_QOS_POLICY_ID) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: %C %d != %d\n"),
      what, DDS::DATA_REPRESENTATION_QOS_POLICY_ID, q[0].policy_id));
    rv = false;
    print = true;
  }
  if (print) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: %C:\n"), what));
    for (CORBA::ULong i = 0; i < cnt; ++i) {
      ACE_ERROR((LM_ERROR, ACE_TEXT(" - id:%u, count:%d\n"), q[i].policy_id, q[i].count));
    }
  }
  return rv;
}

Test::CheckMatchResult Test::check_match(DDS::DataReader* reader, DDS::DataWriter* writer, bool expect_match)
{
  const CORBA::Long expected_match = expect_match ? 1 : 0;
  const CORBA::Long incompatible_qos = expect_match ? 0 : 1;
  DDS::StatusCondition_var writer_condition = writer->get_statuscondition();
  DDS::StatusCondition_var reader_condition = reader->get_statuscondition();
  writer_condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS);
  reader_condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(writer_condition);
  ws->attach_condition(reader_condition);
  DDS::ConditionSeq conditions;
  bool reader_done = false;
  bool writer_done = false;
  const DDS::Duration_t max_wait_time = {10, 0};
  while (!reader_done && !writer_done) {
    DDS::ReturnCode_t rc = ws->wait(conditions, max_wait_time);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: wait failed: %C\n"), retcode_to_string(rc)));
      return check_match_error;
    }
    bool unexpected_result = false;
    for (CORBA::ULong i = 0; i < conditions.length(); ++i) {
      if (conditions[i] == writer_condition) {
        // Check if we matched
        DDS::PublicationMatchedStatus writer_match;
        rc = writer->get_publication_matched_status(writer_match);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: get_publication_matched_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_error;
        }
        if (expected_match != writer_match.total_count_change) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: publication match %d != %d\n"),
            expected_match, writer_match.total_count));
          unexpected_result = true;
        }
        // Check if QoS was deemed incompatible
        DDS::OfferedIncompatibleQosStatus writer_qos_fail;
        rc = writer->get_offered_incompatible_qos_status(writer_qos_fail);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: get_offered_incompatible_qos_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_error;
        }
        if (incompatible_qos != writer_qos_fail.total_count) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: offered incompatible qos %d != %d\n"),
            incompatible_qos, writer_qos_fail.total_count));
          unexpected_result = true;
        }
        if (!check_policies("offered incompatible qos", expect_match, writer_qos_fail.policies)) {
          unexpected_result = true;
        }
        writer_done = true;
      } else if (conditions[i] == reader_condition) {
        // Check if we matched
        DDS::SubscriptionMatchedStatus reader_match;
        rc = reader->get_subscription_matched_status(reader_match);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: get_subscription_matched_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_error;
        }
        if (expected_match != reader_match.total_count_change) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: subscription match %d != %d\n"),
            expected_match, reader_match.total_count));
          unexpected_result = true;
        }
        // Check if QoS was deemed incompatible
        DDS::RequestedIncompatibleQosStatus reader_qos_fail;
        rc = reader->get_requested_incompatible_qos_status(reader_qos_fail);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: get_requested_incompatible_qos_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_error;
        }
        if (incompatible_qos != reader_qos_fail.total_count) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: requested incompatible qos %d != %d\n"),
            incompatible_qos, reader_qos_fail.total_count));
          unexpected_result = true;
        }
        if (!check_policies("requested incompatible qos", expect_match, reader_qos_fail.policies)) {
          unexpected_result = true;
        }
        reader_done = true;
      }
    }
    if (unexpected_result) {
      return check_match_unexpected;
    }
  }
  return check_match_expected;
}

void Test::add_result(bool passed)
{
  if (passed) {
    ++cases_passed_;
  }
  ++cases_total_;
}

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    Test test(argc, argv);
    return test.run();
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ACE_TMAIN() ERROR: exception caught\n")));
    return 1;
  }
}
