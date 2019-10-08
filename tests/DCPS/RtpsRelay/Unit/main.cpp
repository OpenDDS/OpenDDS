#include "tools/rtpsrelay/lib/Name.h"
#include "tools/rtpsrelay/lib/QosIndex.h"

#include "dds/DCPS/Service_Participant.h"

#include <iostream>

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

#define ASSERT_MATCHED do { \
  std::set<Reader*> actual_readers, expected_readers;                   \
  index.get_readers(&writer, actual_readers);                           \
  expected_readers.insert(&reader);                                     \
  if (actual_readers != expected_readers) {                             \
    std::cout << "ERROR: " <<  __func__ << " failed: writer does not match reader" << std::endl; \
    status = EXIT_FAILURE;                                              \
  }                                                                     \
  std::set<Writer*> actual_writers, expected_writers;                   \
  index.get_writers(&reader, actual_writers);                           \
  expected_writers.insert(&writer);                                     \
  if (actual_writers != expected_writers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: reader does not match writer" << std::endl; \
    status = EXIT_FAILURE; \
 } \
} while(0);

#define ASSERT_NOT_MATCHED do { \
  std::set<Reader*> actual_readers, expected_readers;                   \
  index.get_readers(&writer, actual_readers);                           \
  if (actual_readers != expected_readers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: writer matches reader" << std::endl; \
    status = EXIT_FAILURE;                                              \
  }                                                                     \
  std::set<Writer*> actual_writers, expected_writers;                   \
  index.get_writers(&reader, actual_writers);                           \
  if (actual_writers != expected_writers) {                             \
    std::cout << "ERROR: " << __func__ << " failed: reader matches writer" << std::endl; \
    status = EXIT_FAILURE; \
 } \
} while(0);

template <typename Index>
void writer_then_matched_reader(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bjec[!s] *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;
}

template <typename Index>
void reader_then_matched_writer(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&reader);
  index.insert(&writer);

  ASSERT_MATCHED;
}

template <typename Index>
void matched_then_writer_changes_reliability(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.data_writer_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

  index.reinsert(&writer, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_reliability(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.data_reader_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  index.reinsert(&reader, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_writer_changes_partition(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.publisher_qos().partition.name[0] = "Object";

  index.reinsert(&writer, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_partition(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.subscriber_qos().partition.name[0] = "Object";

  index.reinsert(&reader, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_writer_changes_topic(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.topic_name("a new topic");

  index.reinsert(&writer, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_changes_topic(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.topic_name("a new topic");

  index.reinsert(&reader, entry);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void unmatched_then_writer_changes_reliability(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.data_writer_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  index.reinsert(&writer, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void unmatched_then_reader_changes_reliability(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.data_writer_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.data_reader_qos().reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "?bject *, [Ii]nc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.data_reader_qos().reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

  index.reinsert(&reader, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void unmatched_then_writer_changes_partition(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "OCI";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.publisher_qos().partition.name[0] = "OCI";

  index.reinsert(&writer, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void unmatched_then_reader_changes_partition(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "OCI";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";

  index.reinsert(&reader, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void unmatched_then_writer_changes_topic(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("wrong topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "O*C*I*";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::WriterEntry entry(writer.writer_entry);
  entry.topic_name("the topic");

  index.reinsert(&writer, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void unmatched_then_reader_changes_topic(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("wrong topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_NOT_MATCHED;

  RtpsRelay::ReaderEntry entry(reader.reader_entry);
  entry.topic_name("the topic");

  index.reinsert(&reader, entry);

  ASSERT_MATCHED;
}

template <typename Index>
void matched_then_writer_disappears(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "Object Computing, Inc.";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "O*C*I*";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  index.erase(&writer);

  ASSERT_NOT_MATCHED;
}

template <typename Index>
void matched_then_reader_disappears(int& status)
{
  Writer writer(RtpsRelay::WriterEntry(), true);
  writer.writer_entry.topic_name("the topic");
  writer.writer_entry.type_name("the type");
  writer.writer_entry.data_writer_qos(TheServiceParticipant->initial_DataWriterQos());
  writer.writer_entry.publisher_qos(TheServiceParticipant->initial_PublisherQos());
  writer.writer_entry.publisher_qos().partition.name.length(1);
  writer.writer_entry.publisher_qos().partition.name[0] = "O*C*I*";

  Reader reader(RtpsRelay::ReaderEntry(), true);
  reader.reader_entry.topic_name("the topic");
  reader.reader_entry.type_name("the type");
  reader.reader_entry.data_reader_qos(TheServiceParticipant->initial_DataReaderQos());
  reader.reader_entry.subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
  reader.reader_entry.subscriber_qos().partition.name.length(1);
  reader.reader_entry.subscriber_qos().partition.name[0] = "Object Computing, Inc.";

  Index index;
  index.insert(&writer);
  index.insert(&reader);

  ASSERT_MATCHED;

  index.erase(&reader);

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

int main()
{
  int status = 0;

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

  qos_index_test<NoIndex>(status);
  qos_index_test<PartitionIndex<NoIndex> >(status);
  qos_index_test<TopicIndex<NoIndex> >(status);
  qos_index_test<TopicIndex<PartitionIndex<NoIndex> > >(status);
  qos_index_test<PartitionIndex<TopicIndex<NoIndex> > >(status);

  if (!status) {
    std::cout << "SUCCESS" << std::endl;
  }

  return status;
}
