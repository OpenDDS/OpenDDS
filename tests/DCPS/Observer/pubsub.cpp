#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>
#include <tests/Utils/DistributedConditionSet.h>
#include <tests/DCPS/common/TestSupport.h>

#include <dds/DCPS/EntityImpl.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace OpenDDS::DCPS;

class TheObserver : public Observer {
public:
  TheObserver(DistributedConditionSet_rch dcs,
              const String& actor)
    : dcs_(dcs)
    , actor_(actor)
  {}

  void on_enabled(DDS::DataWriter_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
  }

  void on_enabled(DDS::DataReader_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
  }

  void on_deleted(DDS::DataWriter_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
  }

  void on_deleted(DDS::DataReader_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
  }

  void on_qos_changed(DDS::DataWriter_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
  }

  void on_qos_changed(DDS::DataReader_ptr)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
  }

  void on_associated(DDS::DataWriter_ptr, const GUID_t& /* readerId */)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
  }

  void on_associated(DDS::DataReader_ptr, const GUID_t& /* writerId */)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
  }

  void on_disassociated(DDS::DataWriter_ptr, const GUID_t& /* readerId */)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
  }

  void on_disassociated(DDS::DataReader_ptr, const GUID_t& /* writerId */)
  {
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
  }

  void on_sample_sent(DDS::DataWriter_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_sample_received(DDS::DataReader_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_sample_read(DDS::DataReader_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_sample_taken(DDS::DataReader_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_disposed(DDS::DataWriter_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_disposed(DDS::DataReader_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_unregistered(DDS::DataWriter_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_writer_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

  void on_unregistered(DDS::DataReader_ptr, const Sample& s)
  {
    ACE_UNUSED_ARG(s);
    static size_t count = 0;
    dcs_->post(actor_, String(__func__) + "_reader_" + to_dds_string(count++));
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << OpenDDS::DCPS::to_json(s) << std::endl;
#endif
  }

private:
  DistributedConditionSet_rch dcs_;
  const String actor_;
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();
  const String DRIVER = "DRIVER";
  const String PARTICIPANT = "PARTICIPANT";
  const String PUBLISHER = "PUBLISHER";
  const String WRITER = "WRITER";
  const String SUBSCRIBER = "SUBSCRIBER";
  const String READER = "READER";

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant = dpf->create_participant(1066,
                                                                   PARTICIPANT_QOS_DEFAULT,
                                                                   DDS::DomainParticipantListener::_nil(),
                                                                   DEFAULT_STATUS_MASK);
  Messenger::MessageTypeSupport_var ts(new Messenger::MessageTypeSupportImpl);

  ts->register_type(participant.in(), "TestObserver Type");
  DDS::Topic_var topic = participant->create_topic("TestObserver Topic",
                                                   "TestObserver Type",
                                                   TOPIC_QOS_DEFAULT,
                                                   DDS::TopicListener::_nil(),
                                                   DEFAULT_STATUS_MASK);

  RcHandle<TheObserver> participant_observer = make_rch<TheObserver>(dcs, PARTICIPANT);
  EntityImpl* participant_entity = dynamic_cast<EntityImpl*>(participant.ptr());
  participant_entity->set_observer(participant_observer, Observer::e_ENABLED | Observer::e_DELETED);

  DDS::Publisher_var publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                               DDS::PublisherListener::_nil(),
                                                               DEFAULT_STATUS_MASK);

  RcHandle<TheObserver> publisher_observer = make_rch<TheObserver>(dcs, PUBLISHER);
  EntityImpl* publisher_entity = dynamic_cast<EntityImpl*>(publisher.ptr());
  publisher_entity->set_observer(publisher_observer, Observer::e_QOS_CHANGED | Observer::e_ASSOCIATED | Observer::e_DISASSOCIATED);

  DDS::DataWriter_var dw = publisher->create_datawriter(topic,
                                                        DATAWRITER_QOS_DEFAULT,
                                                        DDS::DataWriterListener::_nil(),
                                                        DEFAULT_STATUS_MASK);
  Messenger::MessageDataWriter_var writer = Messenger::MessageDataWriter::_narrow(dw);

  RcHandle<TheObserver> writer_observer = make_rch<TheObserver>(dcs, WRITER);
  EntityImpl* writer_entity = dynamic_cast<EntityImpl*>(dw.ptr());
  writer_entity->set_observer(writer_observer, Observer::e_SAMPLE_SENT | Observer::e_DISPOSED | Observer::e_UNREGISTERED);

  DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                  DDS::SubscriberListener::_nil(),
                                                                  DEFAULT_STATUS_MASK);

  RcHandle<TheObserver> subscriber_observer = make_rch<TheObserver>(dcs, SUBSCRIBER);
  EntityImpl* subscriber_entity = dynamic_cast<EntityImpl*>(subscriber.ptr());
  subscriber_entity->set_observer(subscriber_observer, Observer::e_QOS_CHANGED | Observer::e_ASSOCIATED | Observer::e_DISASSOCIATED);

  DDS::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var dr = subscriber->create_datareader(topic,
                                                         reader_qos,
                                                         DDS::DataReaderListener::_nil(),
                                                         DEFAULT_STATUS_MASK);
  Messenger::MessageDataReader_var reader = Messenger::MessageDataReader::_narrow(dr);

  RcHandle<TheObserver> reader_observer = make_rch<TheObserver>(dcs, READER);
  EntityImpl* reader_entity = dynamic_cast<EntityImpl*>(dr.ptr());
  reader_entity->set_observer(reader_observer, Observer::e_SAMPLE_RECEIVED | Observer::e_SAMPLE_READ | Observer::e_SAMPLE_TAKEN | Observer::e_DISPOSED | Observer::e_UNREGISTERED);

  // Check get observer.
  TEST_ASSERT(writer_entity->get_observer(Observer::e_SAMPLE_SENT) == writer_observer);
  TEST_ASSERT(writer_entity->get_observer(Observer::e_ASSOCIATED) == publisher_observer);
  TEST_ASSERT(writer_entity->get_observer(Observer::e_ENABLED) == participant_observer);
  TEST_ASSERT(writer_entity->get_observer(Observer::e_SAMPLE_READ) == 0);
  TEST_ASSERT(reader_entity->get_observer(Observer::e_SAMPLE_RECEIVED) == reader_observer);
  TEST_ASSERT(reader_entity->get_observer(Observer::e_ASSOCIATED) == subscriber_observer);
  TEST_ASSERT(reader_entity->get_observer(Observer::e_ENABLED) == participant_observer);
  TEST_ASSERT(reader_entity->get_observer(Observer::e_SAMPLE_SENT) == 0);

  // Wait for writer and reader to be enabled.
  dcs->wait_for(DRIVER, PARTICIPANT, "on_enabled_writer_0");
  dcs->wait_for(DRIVER, PARTICIPANT, "on_enabled_reader_0");

  // Wait for writer and reader to be associated.
  dcs->wait_for(DRIVER, PUBLISHER, "on_associated_writer_0");
  dcs->wait_for(DRIVER, SUBSCRIBER, "on_associated_reader_0");

  {
    // Scoped to return samples.

    // Write a sample.
    //Message{"from", "subject", @key subject_id, "text", count, ull, source_pid}
    Messenger::Message message = {"", "Observer", 1, "test", 1, 0, 0};
    writer->write(message, DDS::HANDLE_NIL);

    // Wait for sample to be send and received.
    dcs->wait_for(DRIVER, WRITER, "on_sample_sent_0");
    dcs->wait_for(DRIVER, READER, "on_sample_received_0");

    // Check read and take.
    Messenger::MessageSeq datas;
    DDS::SampleInfoSeq infos;
    reader->read(datas, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    dcs->wait_for(DRIVER, READER, "on_sample_read_0");
    reader->take(datas, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    dcs->wait_for(DRIVER, READER, "on_sample_taken_0");

    // Checking dispose and unrgister.
    // auto disposed is turned on.
    writer->unregister_instance(message, DDS::HANDLE_NIL);
    dcs->wait_for(DRIVER, WRITER, "on_disposed_writer_0");
    dcs->wait_for(DRIVER, READER, "on_disposed_reader_0");
    dcs->wait_for(DRIVER, WRITER, "on_unregistered_writer_0");
    dcs->wait_for(DRIVER, READER, "on_unregistered_reader_0");
  }

  // Change userdata to check for qos changes.
  DDS::OctetSeq user_data;
  user_data.length(1);
  user_data[0] = 'a';

  DDS::DataWriterQos writer_qos;
  writer->get_qos(writer_qos);
  writer_qos.user_data.value= user_data;
  writer->set_qos(writer_qos);

  reader->get_qos(reader_qos);
  reader_qos.user_data.value= user_data;
  reader->set_qos(reader_qos);

  dcs->wait_for(DRIVER, PUBLISHER, "on_qos_changed_writer_0");
  dcs->wait_for(DRIVER, SUBSCRIBER, "on_qos_changed_reader_0");

  // Change partition to force disassociation.
  DDS::PublisherQos publisher_qos;
  publisher->get_qos(publisher_qos);
  publisher_qos.partition.name.length(1);
  publisher_qos.partition.name[0] = "abc";
  publisher->set_qos(publisher_qos);

  dcs->wait_for(DRIVER, PUBLISHER, "on_disassociated_writer_0");
  dcs->wait_for(DRIVER, SUBSCRIBER, "on_disassociated_reader_0");

  // Delete.
  publisher->delete_datawriter(writer);
  subscriber->delete_datareader(reader);
  dcs->wait_for(DRIVER, PARTICIPANT, "on_deleted_writer_0");
  dcs->wait_for(DRIVER, PARTICIPANT, "on_deleted_reader_0");

  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
