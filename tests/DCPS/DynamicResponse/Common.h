#include <tests/Utils/DistributedConditionSet.h>
#include <tests/Utils/WaitForSample.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>

#include <string>

const char ORIGIN[] = "origin";
const char RESPONDER[] = "responder";

const char TOPICS_CREATED[] = "topics created";
const char TOPICS_ANALYZED[] = "topics analyzed";
const char ORIGIN_SAMPLES_AWAY[] = "origin samples away";
const char RESPONDER_SAMPLES_AWAY[] = "responder samples away";
const char DONE[] = "done";

struct Test {
  const char* const name;
  int exit_status;
  DistributedConditionSet_rch dcs;
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var dp;
  DDS::Subscriber_var sub;
  DDS::Publisher_var pub;

  Test(const char* name)
    : name(name)
    , exit_status(0)
    , dcs(OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>())
  {
  }

  ~Test()
  {
    ACE_DEBUG((LM_DEBUG, "%C (%P|%t) shutting down...\n", name));

    dp->delete_contained_entities();
    dpf->delete_participant(dp);
    TheServiceParticipant->shutdown();

    ACE_DEBUG((LM_DEBUG, "%C (%P|%t) shutdown\n", name));
  }

  bool init(int& argc, ACE_TCHAR* argv[])
  {
    using namespace OpenDDS::DCPS;
    using namespace OpenDDS::RTPS;

    dpf = TheParticipantFactoryWithArgs(argc, argv);

    const DDS::DomainId_t domain = 12;

    RtpsDiscovery_rch disc = make_rch<RtpsDiscovery>("rtps_disc");
    disc->use_xtypes(RtpsDiscoveryConfig::XTYPES_COMPLETE);
    TheServiceParticipant->add_discovery(static_rchandle_cast<Discovery>(disc));
    TheServiceParticipant->set_repo_domain(domain, disc->key());

    TransportConfig_rch transport_config =
      TheTransportRegistry->create_config("default_rtps_transport_config");
    TransportInst_rch transport_inst =
      TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
    transport_config->instances_.push_back(transport_inst);
    TheTransportRegistry->global_config(transport_config);

    dp = dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!dp) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_participant failed!\n", name));
      return false;
    }

    sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!sub) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_publisher failed!\n", name));
      return false;
    }

    pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!pub) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: pub: create_publisher failed!\n", name));
      return false;
    }

    return true;
  }

  bool init_topic(DDS::TypeSupport_var& ts, DDS::Topic_var& topic)
  {
    if (!check_rc(ts->register_type(dp, ""), "register_type")) {
      return false;
    }

    CORBA::String_var type_name = ts->get_type_name();
    topic = dp->create_topic(type_name, type_name, TOPIC_QOS_DEFAULT, 0,
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_topic failed!\n", name));
      return false;
    }

    return true;
  }

  template <typename DataWriterType>
  typename DataWriterType::_var_type create_writer(DDS::Topic_var& topic)
  {
    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic, qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    typename DataWriterType::_var_type dw2 = DataWriterType::_narrow(dw);
    if (!dw2) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_datawriter failed!\n", name));
      exit_status = 1;
    }
    return dw2;
  }

  template <typename DataReaderType>
  typename DataReaderType::_var_type create_reader(DDS::Topic_var& topic)
  {
    DDS::DataReaderQos qos;
    sub->get_default_datareader_qos(qos);
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    DDS::DataReader_var dr =
      sub->create_datareader(topic, qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    typename DataReaderType::_var_type dr2 = DataReaderType::_narrow(dr);
    if (!dr2) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_datareader failed!\n", name));
      exit_status = 1;
    }
    return dr2;
  }

  bool check_rc(DDS::ReturnCode_t rc, const std::string& what)
  {
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: %C: %C\n",
        name, what.c_str(), OpenDDS::DCPS::retcode_to_string(rc)));
      exit_status = 1;
      return false;
    }
    return true;
  }

  void wait_for(const std::string& actor, const std::string& what)
  {
    dcs->wait_for(name, actor, what);
  }

  void post(const std::string& what)
  {
    dcs->post(name, what);
  }
};

template <typename TopicType>
struct Topic {
  typedef OpenDDS::DCPS::DDSTraits<TopicType> Traits;

  Test& t;
  std::string name;
  DDS::TypeSupport_var ts;
  DDS::Topic_var topic;
  typedef typename Traits::DataWriterType DataWriterType;
  typename DataWriterType::_var_type writer;
  typedef typename Traits::DataReaderType DataReaderType;
  typename DataReaderType::_var_type reader;
  typedef typename Traits::MessageSequenceType SeqType;

  Topic(Test& t): t(t) {}

  bool init()
  {
    using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

    ts = new typename Traits::TypeSupportImplType();
    if (!t.init_topic(ts, topic)) {
      return false;
    }

    CORBA::String_var type_name = ts->get_type_name();
    name = type_name.in();

    DDS::DataReader_var dr =
      t.sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0 ,DEFAULT_STATUS_MASK);
    reader = DataReaderType::_narrow(dr);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_datareader failed!\n", t.name));
      return false;
    }

    writer = t.create_writer<DataWriterType>(topic);

    return writer;
  }

  bool init_bit(const char* name)
  {
    DDS::Subscriber_var bit_subscriber = t.dp->get_builtin_subscriber();
    DDS::DataReader_var dr = bit_subscriber->lookup_datareader(name);
    reader = DataReaderType::_narrow(dr);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) failed to get %C datareader\n", name));
      return false;
    }
    return true;
  }

  bool write(const TopicType& msg)
  {
    return writer->write(msg, DDS::HANDLE_NIL) == DDS::RETCODE_OK;
  }

  bool wait_dr_match(int count) const
  {
    DDS::DataReader_var dr = DDS::DataReader::_duplicate(reader);
    if (Utils::wait_match(dr, count, Utils::EQ)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): Error waiting for match for dr\n"));
      return false;
    }
    return true;
  }

  bool read_multiple(SeqType& seq)
  {
    DDS::DataReader_var dr = DDS::DataReader::_duplicate(reader);
    Utils::waitForSample(dr);
    DDS::SampleInfoSeq info;
    return t.check_rc(reader->read(seq, info, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE), "read failed");
  }

  bool read_one(TopicType& msg)
  {
    DDS::DataReader_var dr = DDS::DataReader::_duplicate(reader);
    ACE_DEBUG((LM_DEBUG, "%C (%P|%t) waiting for sample on %C\n", t.name, name.c_str()));
    Utils::waitForSample(dr);
    DDS::SampleInfo info;
    return t.check_rc(reader->read_next_sample(msg, info), "read failed");
  }
};
