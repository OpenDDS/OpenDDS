#include "tools/rtpsrelay/lib/Name.h"
#include "tools/rtpsrelay/lib/QosIndex.h"

#include "dds/DCPS/Service_Participant.h"

#include <iostream>

using namespace RtpsRelay;

void test_valid(int& status, const std::string& s, const Name& expected)
{
  Name actual(s);
  if (!actual.is_valid()) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ":  expected name to be valid but it wasn't" << std::endl;
    status = EXIT_FAILURE;
    return;
  }

  if (actual != expected) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ':'
              << " expected=" << expected << " pattern=" << std::boolalpha << expected.is_pattern() << " valid=" << std::boolalpha << expected.is_valid()
              << " actual=" << actual << " pattern=" << std::boolalpha <<  actual.is_pattern() << " valid=" << std::boolalpha << actual.is_valid() << std::endl;
    status = EXIT_FAILURE;
    return;
  }
}

void test_invalid(int& status, const std::string& s)
{
  Name actual(s);
  if (actual.is_valid()) {
    std::cout << "ERROR: Test " << '\'' << s << '\'' << ":  expected name to be invalid but it wasn't" << std::endl;
    status = EXIT_FAILURE;
    return;
  }
}

#define ASSERT_MATCHED(x) do {                                  \
  GuidSet expected_guids;                                       \
  expected_guids.insert(x->participant_guid());                 \
  if (guids != expected_guids) { \
    std::cout << "ERROR: " <<  __func__ << " failed: guids do not match" << std::endl; \
    status = EXIT_FAILURE;                                              \
  } \
  ReaderSet actual_readers, expected_readers;                           \
  index.get_readers(writer, actual_readers);                            \
  expected_readers.insert(reader);                                      \
  if (actual_readers != expected_readers) {                             \
    std::cout << "ERROR: " <<  __func__ << " failed: writer does not match reader" << std::endl; \
    status = EXIT_FAILURE;                                              \
  }                                                                     \
  WriterSet actual_writers, expected_writers;                           \
  index.get_writers(reader, actual_writers);                           \
  expected_writers.insert(writer);                                     \
  if (actual_writers != expected_writers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: reader does not match writer" << std::endl; \
    status = EXIT_FAILURE; \
 } \
} while(0);

#define ASSERT_NOT_MATCHED do { \
  if (!guids.empty()) {                    \
    std::cout << "ERROR: " <<  __func__ << " failed: guids should be empty" << std::endl; \
    status = EXIT_FAILURE;                                              \
  } \
  ReaderSet actual_readers, expected_readers;                   \
  index.get_readers(writer, actual_readers);                           \
  if (actual_readers != expected_readers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: writer matches reader" << std::endl; \
    status = EXIT_FAILURE;                                              \
  }                                                                     \
  WriterSet actual_writers, expected_writers;                   \
  index.get_writers(reader, actual_writers);                           \
  if (actual_writers != expected_writers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: reader matches writer" << std::endl; \
    status = EXIT_FAILURE; \
 } \
} while(0);

template <typename Index>
void writer_then_matched_reader(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bjec[!s] *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);
}

template <typename Index>
void reader_then_matched_writer(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(reader, guids);
  index.insert(writer, guids);

  ASSERT_MATCHED(reader);
}

template <typename Index>
void matched_then_writer_changes_reliability(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.data_writer_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

  guids.clear();
  index.reinsert(writer, writer_entry, guids);
  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_reliability(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.data_reader_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  guids.clear();
  index.reinsert(reader, reader_entry, guids);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_writer_changes_partition(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  writer_entry.publisher_qos().partition.name[0] = "Object";

  guids.clear();
  index.reinsert(writer, writer_entry, guids);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_partition(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  reader_entry.subscriber_qos().partition.name[0] = "Object";

  guids.clear();
  index.reinsert(reader, reader_entry, guids);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_writer_changes_topic(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  writer_entry.topic_name("a new topic");

  guids.clear();
  index.reinsert(writer, writer_entry, guids);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_topic(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  reader_entry.topic_name("a new topic");

  guids.clear();
  index.reinsert(reader, reader_entry, guids);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void unmatched_then_writer_changes_reliability(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  writer_entry.data_writer_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  index.reinsert(writer, writer_entry, guids);

  ASSERT_MATCHED(reader);
}

template <typename Index>
void unmatched_then_reader_changes_reliability(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  reader_entry.data_reader_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

  index.reinsert(reader, reader_entry, guids);

  ASSERT_MATCHED(writer);
}

template <typename Index>
void unmatched_then_writer_changes_partition(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "OCI";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  writer_entry.publisher_qos().partition.name[0] = "OCI";

  index.reinsert(writer, writer_entry, guids);

  ASSERT_MATCHED(reader);
}

template <typename Index>
void unmatched_then_reader_changes_partition(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "OCI";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  reader_entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";

  index.reinsert(reader, reader_entry, guids);

  ASSERT_MATCHED(writer);
}

template <typename Index>
void unmatched_then_writer_changes_topic(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("wrong topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "O*C*I*";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  writer_entry.topic_name("the topic");

  index.reinsert(writer, writer_entry, guids);

  ASSERT_MATCHED(reader);
}

template <typename Index>
void unmatched_then_reader_changes_topic(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("wrong topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_NOT_MATCHED;

  reader_entry.topic_name("the topic");

  index.reinsert(reader, reader_entry, guids);

  ASSERT_MATCHED(writer);
}

template <typename Index>
void matched_then_writer_disappears(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "O*C*I*";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);

  ASSERT_MATCHED(writer);

  index.erase(writer);

  guids.clear();

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_disappears(int& status)
{
  WriterEntry writer_entry;
  writer_entry.topic_name("the topic");
  writer_entry.type_name("the type");
  writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer_entry.publisher_qos().partition.name.length(1);
  writer_entry.publisher_qos().partition.name[0] = "O*C*I*";
  WriterPtr writer(new Writer(writer_entry));

  ReaderEntry reader_entry;
  reader_entry.topic_name("the topic");
  reader_entry.type_name("the type");
  reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader_entry.subscriber_qos().partition.name.length(1);
  reader_entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";
  ReaderPtr reader(new Reader(reader_entry));

  Index index;
  GuidSet guids;
  index.insert(writer, guids);
  index.insert(reader, guids);
  ASSERT_MATCHED(writer);

  index.erase(reader);

  guids.clear();

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void qos_index_test(int& status)
{
  writer_then_matched_reader<Index>(status);
  reader_then_matched_writer<Index>(status);

  matched_then_writer_changes_reliability<Index>(status);
  matched_then_reader_changes_reliability<Index>(status);
  matched_then_writer_changes_partition<Index>(status);
  matched_then_reader_changes_partition<Index>(status);
  matched_then_writer_changes_topic<Index>(status);
  matched_then_reader_changes_topic<Index>(status);

  unmatched_then_writer_changes_reliability<Index>(status);
  unmatched_then_reader_changes_reliability<Index>(status);
  unmatched_then_writer_changes_partition<Index>(status);
  unmatched_then_reader_changes_partition<Index>(status);
  unmatched_then_writer_changes_topic<Index>(status);
  unmatched_then_reader_changes_topic<Index>(status);

  matched_then_writer_disappears<Index>(status);
  matched_then_reader_disappears<Index>(status);
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  int status = EXIT_SUCCESS;

  std::set<char> digits;
  for (char c = '0'; c <= '9'; ++c) {
    digits.insert(c);
  }

  std::set<char> abc;
  for (char c = 'a'; c <= 'c'; ++c) {
    abc.insert(c);
  }

  {
    Name expected;
    test_valid(status, "", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    test_valid(status, "a", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid(status, "?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('?'));
    test_valid(status, "\\?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid(status, "*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('*'));
    test_valid(status, "\\*", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('\\'));
    test_valid(status, "\\\\", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(']'));
    test_valid(status, "]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom('a'));
    expected.push_back(Atom('b'));
    expected.push_back(Atom('c'));
    test_valid(status, "abc", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    expected.push_back(Atom('.'));
    expected.push_back(Atom(Atom::WILDCARD));
    test_valid(status, "*.?", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, abc));
    test_valid(status, "[abc]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid(status, "[0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(false, digits));
    test_valid(status, "[091-8]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(true, digits));
    test_valid(status, "[!0-9]", expected);
  }

  {
    Name expected;
    expected.push_back(Atom(Atom::GLOB));
    test_valid(status, "**", expected);
  }

  test_invalid(status, "\\");
  test_invalid(status, "[");
  test_invalid(status, "[]");
  test_invalid(status, "[a-]");
  test_invalid(status, "[b-a]");

  try {
    qos_index_test<NoIndex>(status);
    qos_index_test<PartitionIndex<NoIndex> >(status);
    qos_index_test<TopicIndex<NoIndex> >(status);
    qos_index_test<TopicIndex<PartitionIndex<NoIndex> > >(status);
    qos_index_test<PartitionIndex<TopicIndex<NoIndex> > >(status);
  } catch (const CORBA::BAD_PARAM&) {
    std::cout << "Exception" << std::endl;
    status = EXIT_FAILURE;
  }

  if (!status) {
    std::cout << "SUCCESS" << std::endl;
  }

  return status;
}
