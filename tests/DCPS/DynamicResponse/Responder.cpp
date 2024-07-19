#include "Common.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/XTypes/DynamicTypeSupport.h>
#include <dds/DCPS/XTypes/DynamicDataFactory.h>

#include <dds/DdsDcpsCoreTypeSupportC.h>

#include <string>

struct DynamicTopic {
  DynamicTopic()
  : found(false)
  , get_dynamic_type_rc(DDS::RETCODE_OK)
  {
  }

  bool found;
  DDS::ReturnCode_t get_dynamic_type_rc;
  DDS::DynamicType_var type;
  DDS::TypeSupport_var ts;
  DDS::Topic_var topic;
  DDS::DynamicDataReader_var reader;
  DDS::DynamicDataWriter_var writer;
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  using OpenDDS::DCPS::retcode_to_string;

  Test t(RESPONDER);
  if (!t.init(argc, argv)) {
    return 1;
  }

  Topic<DDS::PublicationBuiltinTopicData> pub_bit(t);
  if (!pub_bit.init_bit(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC)) {
    return 1;
  }

  typedef std::map<std::string, DynamicTopic> DynamicTopicMap;
  DynamicTopicMap topics;
  topics["SimpleStruct"] = DynamicTopic();
  topics["SimpleUnion"] = DynamicTopic();
  topics["SimpleStructNotComplete"].get_dynamic_type_rc = DDS::RETCODE_NO_DATA;

  t.wait_for(ORIGIN, TOPICS_CREATED);

  size_t found_count = 0;
  while (found_count < topics.size()) {
    ACE_DEBUG((LM_DEBUG, "Found %B/%B topics, waiting...\n", found_count, topics.size()));
    DDS::PublicationBuiltinTopicDataSeq pub_bit_seq;
    if (!pub_bit.read_multiple(pub_bit_seq)) {
      return 1;
    }
    for (unsigned i = 0; i < pub_bit_seq.length(); ++i) {
      DDS::PublicationBuiltinTopicData& pb = pub_bit_seq[i];
      ACE_DEBUG((LM_DEBUG, "Learned about topic \"%C\" type \"%C\"\n",
        pb.topic_name.in(), pb.type_name.in()));

      DynamicTopicMap::iterator it = topics.find(pb.topic_name.in());
      if (it == topics.end()) {
        ACE_ERROR((LM_ERROR, "ERROR: Unexpected topic %C\n", pb.topic_name.in()));
      } else {
        DynamicTopic& topic = it->second;
        if (topic.found) {
          continue;
        }
        topic.found = true;
        ++found_count;
        const DDS::ReturnCode_t rc =
          TheServiceParticipant->get_dynamic_type(topic.type, t.dp, pb.key);
        if (rc != topic.get_dynamic_type_rc) {
          ACE_ERROR((LM_ERROR, "ERROR: Expected %C, but got %C\n",
            retcode_to_string(topic.get_dynamic_type_rc), retcode_to_string(rc)));
          t.exit_status = 1;
        }
        if (rc != DDS::RETCODE_OK) {
          continue;
        }

        topic.ts = new DDS::DynamicTypeSupport(topic.type);
        if (!t.init_topic(topic.ts, topic.topic)) {
          t.exit_status = 1;
          continue;
        }

        topic.reader = t.create_reader<DDS::DynamicDataReader>(topic.topic);
        DDS::DataReader_var dr = DDS::DataReader::_duplicate(topic.reader);
        if (Utils::wait_match(dr, 1, Utils::EQ)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): Error waiting for match for dr\n"));
          continue;
        }

        topic.writer = t.create_writer<DDS::DynamicDataWriter>(topic.topic);

        DDS::DataWriter_var dw = DDS::DataWriter::_duplicate(topic.writer);
        if (Utils::wait_match(dw, 2, Utils::EQ)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): Error waiting for match for dw\n"));
          continue;
        }
      }
    }
  }
  ACE_DEBUG((LM_DEBUG, "Found all %B topics\n", topics.size()));

  t.post(TOPICS_ANALYZED);

  t.wait_for(ORIGIN, ORIGIN_SAMPLES_AWAY);

  for (DynamicTopicMap::iterator it = topics.begin(); it != topics.end(); ++it) {
    DynamicTopic& topic = it->second;
    if (topic.type) {
      DDS::DataReader_var dr = DDS::DataReader::_duplicate(topic.reader);
      ACE_DEBUG((LM_DEBUG, "%C (%P|%t) waiting for sample on %C\n", RESPONDER, it->first.c_str()));
      Utils::waitForSample(dr);
      DDS::DynamicData_var dd;
      DDS::SampleInfo info;
      if (!t.check_rc(topic.reader->read_next_sample(dd, info), "read failed")) {
        ACE_DEBUG((LM_ERROR, "ERROR: read_next_sample failed\n"));
        t.exit_status = 1;
      }

      const DDS::TypeKind tk = topic.type->get_kind();
      if (!t.check_rc(dd->set_string_value(0,
          tk == OpenDDS::XTypes::TK_STRUCTURE ? "Hello struct" : "Hello union"),
          "set_string_value failed")) {
        t.exit_status = 1;
        continue;
      }
      if (!t.check_rc(topic.writer->write(dd, DDS::HANDLE_NIL), "write failed")) {
        t.exit_status = 1;
      }
      const DDS::Duration_t duration = { 30, 0 };
      if (topic.writer->wait_for_acknowledgments(duration) != DDS::RETCODE_OK) {
        ACE_DEBUG((LM_ERROR, "ERROR: wait_for_acknowledgments failed\n"));
        t.exit_status = 1;
      }
    }
  }

  t.post(RESPONDER_SAMPLES_AWAY);

  t.wait_for(ORIGIN, DONE);

  return t.exit_status;
}
