/**
 * This test checks how data representation annotations and QoS work together.
 * The gist of it is that `data_representation` annotations on topic types can
 * be overridden by `representation` topic QoS which can be overridden by
 * `representation` reader and writer QoS.
 *
 * For each test case the test will create a reader and writer, see if they
 * match or else incompatible QoS was flagged, then destroy the pair. See the
 * main function for the actual test cases.
 */
#include "DataRepresentationTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include <vector>
#include <sstream>

using OpenDDS::DCPS::retcode_to_string;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

const int domain = 143;
const DDS::Duration_t max_wait_time = {10, 0};

typedef std::vector<DDS::DataRepresentationId_t> Reprs;

void copy_reprs(const Reprs& reprs, DDS::DataRepresentationQosPolicy& qos)
{
  size_t repr_count = reprs.size();
  if (repr_count) {
    qos.value.length(repr_count);
    for (CORBA::ULong i = 0; i != repr_count; ++i) {
      qos.value[i] = reprs[i];
    }
  }
}

std::string reprs_to_string(const Reprs& reprs)
{
  size_t repr_count = reprs.size();
  if (repr_count) {
    std::stringstream ss;
    const Reprs::const_iterator begin = reprs.begin();
    bool first = true;
    for (Reprs::const_iterator i = begin; i != reprs.end(); ++i) {
      if (first) {
        first = false;
      } else {
        ss << ", ";
      }
      if (*i == DDS::XCDR_DATA_REPRESENTATION) {
        ss << "XCDR1";
      } else if (*i == DDS::XCDR2_DATA_REPRESENTATION) {
        ss << "XCDR2";
      } else if (*i == DDS::XML_DATA_REPRESENTATION) {
        ss << "XML";
      } else if (*i == OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION) {
        ss << "UNALIGNED CDR";
      } else {
        ss << "Unknown Value " << *i;
      }
    }
    return ss.str();
  }
  return "Default";
}

template<typename Type>
class RegisteredType {
public:
  typedef typename OpenDDS::DCPS::DDSTraits<Type> TypeTraits;
  typedef typename TypeTraits::TypeSupportType TypeSupport;
  typedef typename TypeSupport::_var_type TypeSupportVar;
  typedef typename TypeTraits::TypeSupportTypeImpl TypeSupportImpl;

  TypeSupportVar type_support_var_;

  RegisteredType()
  : type_support_var_(0)
  {
  }

  bool register_type(DDS::DomainParticipant* participant) {
    type_support_var_ = new TypeSupportImpl;
    DDS::ReturnCode_t rc = type_support_var_->register_type(participant, "");
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
        ACE_TEXT("RegisterType<%C>::register_type failed: %C\n"),
        TypeTraits::type_name(), retcode_to_string(rc)));
      return false;
    }
    return true;
  }

  static const char* type_name()
  {
    return TypeTraits::type_name();
  }
};

DDS::Topic* create_topic(DDS::DomainParticipant* participant,
  const char* topic_name, const char* type_name, const Reprs& reprs)
{
  DDS::TopicQos qos;
  DDS::ReturnCode_t rc = participant->get_default_topic_qos(qos);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("get_default_topic_qos failed!\n")));
    return 0;
  }
  copy_reprs(reprs, qos.representation);

  DDS::Topic* topic = participant->create_topic(topic_name, type_name,
    qos, 0, DEFAULT_STATUS_MASK);
  return topic;
}

DDS::DataReader* create_reader(DDS::Subscriber* subscriber,
  DDS::Topic* topic, const Reprs& reprs = Reprs())
{
  DDS::DataReaderQos qos;
  DDS::ReturnCode_t rc = subscriber->get_default_datareader_qos(qos);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("get_default_datareader_qos failed!\n")));
    return 0;
  }
  copy_reprs(reprs, qos.representation);

  DDS::DataReader* reader = subscriber->create_datareader(
    topic, qos, 0, DEFAULT_STATUS_MASK);
  return reader;
}

DDS::DataWriter* create_writer(DDS::Publisher* publisher,
  DDS::Topic* topic, const Reprs& reprs = Reprs())
{
  DDS::DataWriterQos qos;
  DDS::ReturnCode_t rc = publisher->get_default_datawriter_qos(qos);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("get_default_datawriter_qos failed!\n")));
    return 0;
  }
  copy_reprs(reprs, qos.representation);

  DDS::DataWriter* writer = publisher->create_datawriter(
    topic, qos, 0, DEFAULT_STATUS_MASK);
  return writer;
}

bool check_policies(const char* what, bool expected_to_match,
  const DDS::QosPolicyCountSeq& policies)
{
  const CORBA::ULong policy_count = policies.length();
  const CORBA::ULong expected_count = expected_to_match ? 0 : 1;
  bool rv = true;
  bool print = false;
  if (policy_count != expected_count) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: ")
      ACE_TEXT("expected %C policy sequence count to be ")
      ACE_TEXT("%d, but it's %d.\n"),
      what, expected_count, policy_count));
    rv = false;
    print = policy_count;

  } else if (policy_count == 1 &&
      policies[0].policy_id != DDS::DATA_REPRESENTATION_QOS_POLICY_ID) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: ")
      ACE_TEXT("expected incompatible policy id to be ")
      ACE_TEXT("%d, but it's %d.\n"),
      what, DDS::DATA_REPRESENTATION_QOS_POLICY_ID, policies[0].policy_id));
    rv = false;
    print = true;
  }

  if (print) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_policies() ERROR: ")
      ACE_TEXT("Listing incompatible QoS policy ids found:")));
    for (CORBA::ULong i = 0; i < policy_count; ++i) {
      ACE_ERROR((LM_ERROR, ACE_TEXT(" - policy_id: %u, count: %d\n"),
        policies[i].policy_id, policies[i].count));
    }
  }

  return rv;
}

enum CheckMatchResult {
  check_match_result_error,
  check_match_result_expected_result,
  check_match_result_unexpected_result
};

CheckMatchResult check_match(DDS::DataReader* reader, DDS::DataWriter* writer,
  bool expected_to_match)
{
  DDS::ReturnCode_t rc;

  const bool expected_match_count = expected_to_match ? 1 : 0;
  const bool expected_qos_fail_count = expected_to_match ? 0 : 1;
  DDS::StatusCondition_var writer_condition = writer->get_statuscondition();
  writer_condition->set_enabled_statuses(
    DDS::PUBLICATION_MATCHED_STATUS | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS);
  DDS::StatusCondition_var reader_condition = reader->get_statuscondition();
  reader_condition->set_enabled_statuses(
    DDS::SUBSCRIPTION_MATCHED_STATUS | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(writer_condition);
  ws->attach_condition(reader_condition);
  DDS::ConditionSeq conditions;
  bool reader_done = false;
  bool writer_done = false;
  while (!reader_done && !writer_done) {
    rc = ws->wait(conditions, max_wait_time);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
        ACE_TEXT("wait failed: %C\n"),
        retcode_to_string(rc)));
      return check_match_result_error;
    }

    bool unexpected_result = false;

    for (CORBA::ULong i = 0; i < conditions.length(); ++i) {
      if (conditions[i] == writer_condition) {
        // Check if we matched
        DDS::PublicationMatchedStatus writer_match;
        rc = writer->get_publication_matched_status(writer_match);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("get_publication_matched_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_result_error;
        }
        if (writer_match.total_count != expected_match_count) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("expected publication matched status total count to be ")
            ACE_TEXT("%d, but it's %d.\n"),
            expected_match_count, writer_match.total_count));
          unexpected_result = true;
        }

        // Check if QoS was deemed incompatible
        DDS::OfferedIncompatibleQosStatus writer_qos_fail;
        rc = writer->get_offered_incompatible_qos_status(writer_qos_fail);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("get_offered_incompatible_qos_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_result_error;
        }
        if (writer_qos_fail.total_count != expected_qos_fail_count) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("expected offered incompatible qos status total count to ")
            ACE_TEXT("be %d, but it's %d.\n"),
            expected_qos_fail_count, writer_qos_fail.total_count));
          unexpected_result = true;
        }
        if (!check_policies("offered incompatible qos", expected_to_match,
            writer_qos_fail.policies)) {
          unexpected_result = true;
        }

        writer_done = true;

      } else if (conditions[i] == reader_condition) {
        // Check if we matched
        DDS::SubscriptionMatchedStatus reader_match;
        rc = reader->get_subscription_matched_status(reader_match);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("get_subscription_matched_status failed: %C\n"),
            retcode_to_string(rc)));
          return check_match_result_error;
        }

        // Check if QoS was deemed incompatible
        DDS::RequestedIncompatibleQosStatus reader_qos_fail;
        rc = reader->get_requested_incompatible_qos_status(reader_qos_fail);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l check_match() ERROR: ")
            ACE_TEXT("get_requested_incompatible_qos_status failed: %C\n"),
            retcode_to_string(rc)));
          unexpected_result = true;
        }
        if (!check_policies("requested incompatible qos", expected_to_match,
            reader_qos_fail.policies)) {
          unexpected_result = true;
        }

        reader_done = true;
      }
    }

    if (unexpected_result) {
      return check_match_result_unexpected_result;
    }
  }

  return check_match_result_expected_result;
}


class Test {
public:
  DDS::DomainParticipant* participant_;
  DDS::Publisher* publisher_;
  DDS::Subscriber* subscriber_;
  std::string topic_name_;
  DDS::Topic_var topic_;
  unsigned cases_passed_;
  unsigned cases_total_;

  Test(DDS::DomainParticipant* participant,
    DDS::Publisher* publisher, DDS::Subscriber* subscriber)
  : participant_(participant)
  , publisher_(publisher)
  , subscriber_(subscriber)
  , topic_(0)
  , cases_passed_(0)
  , cases_total_(0)
  {
  }

  void set_topic(const char* type_name, const Reprs& reprs)
  {
    topic_name_ = std::string(type_name) + " Topic";
    if (reprs.size()) {
      topic_name_ += " " + reprs_to_string(reprs);
    }
    topic_ = create_topic(participant_, topic_name_.c_str(), type_name, reprs);
  }

  void add_result(bool passed)
  {
    if (passed) {
      cases_passed_++;
    }
    cases_total_++;
  }

  bool any_failed() {
    if (cases_passed_ != cases_total_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: %u out of %u cases failed\n"),
        cases_total_ - cases_passed_, cases_total_));
      return true;
    }
    return false;
  }

  void test_case(const Reprs& writer_reprs, const Reprs& reader_reprs,
    bool expected_to_match, bool expect_create_writer = true,
    bool expect_create_reader = true)
  {
    if (!topic_) {
      add_result(false);
      return;
    }

    // Create Writer and Reader
    DDS::DataWriter_var writer = create_writer(publisher_, topic_, writer_reprs);
    if (!writer) {
      if (expect_create_writer) {
        add_result(false);
        return;
      } else {
        add_result(true);
        return;
      }
    }
    DDS::DataReader_var reader = create_reader(subscriber_, topic_, reader_reprs);
    if (!reader) {
      if (expect_create_reader) {
        add_result(false);
        return;
      } else {
        add_result(true);
        return;
      }
    }

    // Check if they matched as expected
    bool passed;
    switch (check_match(reader, writer, expected_to_match)) {
    case check_match_result_expected_result:
      passed = true;
      break;

    case check_match_result_unexpected_result:
      passed = false;
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: ")
        ACE_TEXT("Test case with these settings just failed:\n")
        ACE_TEXT("  Topic: \"%C\"\n")
        ACE_TEXT("  Writer Data Representation QoS: %C\n")
        ACE_TEXT("  Reader Data Representation QoS: %C\n")
        ACE_TEXT("  %Cxpected to match.\n"),
        topic_name_.c_str(),
        reprs_to_string(writer_reprs).c_str(),
        reprs_to_string(reader_reprs).c_str(),
        expected_to_match ? "E" : "NOT e"));
      break;

    case check_match_result_error:
      passed = false;
      break;
    }

    // Cleanup
    DDS::ReturnCode_t rc = publisher_->delete_datawriter(writer);
    if (rc != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: ")
        ACE_TEXT("Failed to get delete writer: %C\n"),
        retcode_to_string(rc)));
      passed = false;
    }
    rc = subscriber_->delete_datareader(reader);
    if (rc != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l test_case() ERROR: ")
        ACE_TEXT("Failed to get delete reader: %C\n"),
        retcode_to_string(rc)));
      passed = false;
    }

    add_result(passed);
  }
};

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  int exit_status = 0;
  try {
    // Setup OpenDDS Parent Entities
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("Participant Factory failed!\n")));
      return 1;
    }
    DDS::DomainParticipant_var participant = dpf->create_participant(
      domain, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!participant) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("create_participant failed!\n")));
      return 1;
    }
    DDS::Subscriber_var subscriber = participant->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!subscriber) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("create_subscriber failed!\n")));
      return 1;
    }
    DDS::Publisher_var publisher = participant->create_publisher(
      PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!publisher) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("create_publisher failed!\n")));
      return 1;
    }

    const Reprs default_repr;

    Reprs xcdr1_repr;
    xcdr1_repr.push_back(DDS::XCDR_DATA_REPRESENTATION);

    Reprs xcdr2_repr;
    xcdr2_repr.push_back(DDS::XCDR2_DATA_REPRESENTATION);

    Reprs unaligned_repr;
    unaligned_repr.push_back(OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION);

    Reprs xml_repr;
    xml_repr.push_back(DDS::XML_DATA_REPRESENTATION);

    Reprs explicit_default_reprs; // Or at least what is default as of writing
    explicit_default_reprs.push_back(DDS::XCDR_DATA_REPRESENTATION);
    explicit_default_reprs.push_back(
      OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION);

    Test test(participant, publisher, subscriber);

    RegisteredType<DefaultedType> defaulted_type;
    if (!defaulted_type.register_type(participant.in())) {
      return 1;
    }

    test.set_topic(defaulted_type.type_name(), default_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false);
    test.test_case(default_repr, xcdr2_repr, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    test.test_case(xcdr1_repr, xcdr1_repr, true);
    test.test_case(explicit_default_reprs, xcdr1_repr, true);
    test.test_case(xcdr1_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, explicit_default_reprs, false);
    test.test_case(explicit_default_reprs, xcdr2_repr, false);

    test.set_topic(defaulted_type.type_name(), xcdr2_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false);
    test.test_case(default_repr, xcdr2_repr, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    test.set_topic(defaulted_type.type_name(), xml_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false);
    test.test_case(default_repr, xcdr2_repr, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    RegisteredType<ExplicitDefaultType> explictly_default_type;
    if (!explictly_default_type.register_type(participant.in())) {
      return 1;
    }

    test.set_topic(explictly_default_type.type_name(), default_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, true, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    test.set_topic(explictly_default_type.type_name(), xcdr2_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, true, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    RegisteredType<Xcdr1Type> xcdr1_type;
    if (!xcdr1_type.register_type(participant.in())) {
      return 1;
    }

    test.set_topic(xcdr1_type.type_name(), default_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, true, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    test.set_topic(xcdr1_type.type_name(), xcdr2_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, true, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);

    RegisteredType<XmlType> xml_type;
    if (!xml_type.register_type(participant.in())) {
      return 1;
    }

    test.set_topic(xml_type.type_name(), default_repr);
    test.test_case(default_repr, default_repr, true, false);
    test.test_case(explicit_default_reprs, default_repr, true, false);
    test.test_case(default_repr, explicit_default_reprs, true, false);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, false);
    test.test_case(xml_repr, default_repr, false, false, false);
    test.test_case(default_repr, xml_repr, false, false);
    test.test_case(xml_repr, xml_repr, true, false);

    test.set_topic(xcdr1_type.type_name(), xml_repr);
    test.test_case(default_repr, default_repr, true);
    test.test_case(explicit_default_reprs, default_repr, true);
    test.test_case(default_repr, explicit_default_reprs, true);
    test.test_case(xcdr2_repr, default_repr, false, false);
    test.test_case(default_repr, xcdr2_repr, false, true, false);
    test.test_case(xml_repr, default_repr, false, false);
    test.test_case(default_repr, xml_repr, false, true, false);
    test.test_case(xml_repr, xml_repr, true, false);

    if (test.any_failed()) {
      exit_status = 1;
    }

    // Clean-up
    DDS::ReturnCode_t rc = participant->delete_contained_entities();
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
        ACE_TEXT("delete_contained_entities failed: %C\n"),
        retcode_to_string(rc)));
      exit_status = 1;
    }
    rc = dpf->delete_participant(participant.in());
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
        ACE_TEXT("delete_participant failed: %C\n"),
        retcode_to_string(rc)));
      exit_status = 1;
    }
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Caught in main()");
    exit_status = 1;
  }

  return exit_status;
}
