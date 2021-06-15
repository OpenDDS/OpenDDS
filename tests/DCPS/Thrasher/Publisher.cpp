
#include "Publisher.h"
#include "ProgressIndicator.h"

#include <FooTypeTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <string>

Publisher::Publisher(const DDS::DomainId_t domainId, std::size_t samples_per_thread, bool durable)
  : domainId_(domainId)
  , samples_per_thread_(samples_per_thread)
  , durable_(durable)
  , mutex_()
  , thread_index_(0)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> Publisher::Publisher\n")));
}

Publisher::~Publisher()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- Publisher::~Publisher\n")));
}

void Publisher::start(const int n_threads, const long flags)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> Publisher::start\n")));
  if (activate(flags, n_threads) != 0) {
    throw std::runtime_error(" ERROR: Publisher::start failed!\n");
  }
}

int Publisher::svc()
{
  int ret = 1;
  std::string pfx("(%P|%t) pub");
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var dp;
  try {
    dpf = TheParticipantFactory;
    if (!dpf) {
      throw std::runtime_error(" ERROR: TheParticipantFactoryd is null!\n");
    }
    // Create Participant
    dp = dpf->create_participant(domainId_, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dp) {
      throw std::runtime_error(" ERROR: create_participant failed!\n");
    }
    const int this_thread_index = get_thread_index(pfx, dp);
    ACE_DEBUG((LM_INFO, (pfx + "->started\n").c_str()));

    // Create Publisher
    DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!pub) {
      throw std::runtime_error(" ERROR: create_publisher failed!\n");
    }
    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(dp.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error(" ERROR: register_type failed!\n");
    }
    // Create Topic (FooTopic)
    DDS::Topic_var topic = dp->create_topic("FooTopic", CORBA::String_var(ts->get_type_name()),
      TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      throw std::runtime_error(" ERROR: create_topic failed!\n");
    }
    // Create DataWriter
    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    if (durable_) {
      qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    }
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
    qos.history.depth = static_cast<CORBA::Long>(samples_per_thread_);
#endif
    DDS::DataWriter_var dw = pub->create_datawriter(topic.in(), qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dw) {
      throw std::runtime_error(" ERROR: create_datawriter failed!\n");
    }
    OpenDDS::DCPS::DataWriterImpl* wi = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dw.in());
    ACE_DEBUG((LM_INFO, (pfx + "  writer id: %C\n").c_str(), OpenDDS::DCPS::LogGuid(wi->get_repo_id()).c_str()));
    FooDataWriter_var foo_writer = FooDataWriter::_narrow(dw);
    if (!foo_writer) {
      throw std::runtime_error(" ERROR: FooDataWriter::_narrow failed!\n");
    }

    if (!durable_) {
      ACE_DEBUG((LM_INFO, (pfx + "->wait_match() before write\n").c_str()));
      Utils::wait_match(dw, 1);
      ACE_DEBUG((LM_INFO, (pfx + "<-match found! before write\n").c_str()));
    }

    // The following is intentionally inefficient to stress various
    // pathways related to publication; we should be especially dull
    // and write only one sample at a time per writer.
    const std::string fmt(pfx + "  %d%% (%d samples sent)\n");
    ProgressIndicator progress(fmt.c_str(), samples_per_thread_);
    Foo foo;
    foo.key = 3;
    foo.x = (float) this_thread_index;
    DDS::InstanceHandle_t instance = foo_writer->register_instance(foo);
    for (std::size_t i = 0; i < samples_per_thread_; ++i) {
      foo.y = (float) i;
      if (foo_writer->write(foo, instance) != DDS::RETCODE_OK) {
        throw std::runtime_error(" ERROR: write failed!\n");
      }
      ++progress;
    }

    if (durable_) {
      ACE_DEBUG((LM_INFO, (pfx + "->wait_match()\n").c_str()));
      Utils::wait_match(dw, 1);
      ACE_DEBUG((LM_INFO, (pfx + "<-match found!\n").c_str()));
    }

    DDS::Duration_t interval = {120, 0};
    ACE_DEBUG((LM_INFO, (pfx + "  waiting for acks\n").c_str()));
    if (DDS::RETCODE_OK != dw->wait_for_acknowledgments(interval)) {
      throw std::runtime_error(" ERROR: timed out waiting for acks!\n");
    }

    ret = 0;
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("caught in Publisher::svc()");
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, (pfx + e.what()).c_str()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, (pfx + " exception\n").c_str()));
  }
  // Clean-up!
  if (dp) {
    ACE_DEBUG((LM_INFO, (pfx + "<-delete_contained_entities\n").c_str()));
    dp->delete_contained_entities();
    if (dpf) {
      ACE_DEBUG((LM_INFO, (pfx + "<-delete_participant\n").c_str()));
      dpf->delete_participant(dp.in());
    }
  }
  return ret;
}

void Publisher::configure_transport(const int thread_index, const std::string& pfx, const DDS::DomainParticipant_var& dp)
{
  // RTPS cannot be shared
  OpenDDS::DCPS::Discovery_rch disc = TheServiceParticipant->get_discovery(domainId_);
  OpenDDS::RTPS::RtpsDiscovery_rch rd = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(disc);
  if (!rd.is_nil()) {
    char config_name[64], inst_name[64];
    ACE_TCHAR nak_depth[8];
    ACE_OS::snprintf(config_name, 64, "cfg_%d", thread_index);
    ACE_OS::snprintf(inst_name, 64, "rtps_%d", thread_index);
    // The 2 is a safety factor to allow for control messages.
    ACE_OS::snprintf(nak_depth, 8, ACE_TEXT("%lu"), 2 * samples_per_thread_);
    ACE_DEBUG((LM_INFO, (pfx + "->transport %C\n").c_str(), config_name));
    OpenDDS::DCPS::TransportConfig_rch config = TheTransportRegistry->create_config(config_name);
    OpenDDS::DCPS::TransportInst_rch inst = TheTransportRegistry->create_inst(inst_name, "rtps_udp");
    ACE_Configuration_Heap ach;
    ACE_Configuration_Section_Key sect_key;
    ach.open();
    ach.open_section(ach.root_section(), ACE_TEXT("not_root"), 1, sect_key);
    ach.set_string_value(sect_key, ACE_TEXT("use_multicast"), ACE_TEXT("0"));
    ach.set_string_value(sect_key, ACE_TEXT("nak_depth"), nak_depth);
    ach.set_string_value(sect_key, ACE_TEXT("heartbeat_period"), ACE_TEXT("200"));
    ach.set_string_value(sect_key, ACE_TEXT("heartbeat_response_delay"), ACE_TEXT("100"));
    inst->load(ach, sect_key);
    config->instances_.push_back(inst);
    TheTransportRegistry->bind_config(config_name, dp);
  }
}

int Publisher::get_thread_index(std::string& pfx, const DDS::DomainParticipant_var& dp)
{
  Lock lock(mutex_);
  int index = thread_index_++;
  char pub_i[8];
  ACE_OS::snprintf(pub_i, 8, "%d", index);
  pfx += pub_i;
  configure_transport(index, pfx, dp);
  return index;
}
