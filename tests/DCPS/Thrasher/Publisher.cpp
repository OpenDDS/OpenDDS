
#include "Publisher.h"
#include "ProgressIndicator.h"

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
#include <stdexcept>

Publisher::Publisher(const long domain_id, std::size_t samples_per_thread, bool durable, const int thread_index)
  : domain_id_(domain_id)
  , samples_per_thread_(samples_per_thread)
  , durable_(durable)
  , thread_index_(thread_index)
  , pfx_(create_pfx(thread_index_))
{
  try {
    dpf_ = TheParticipantFactory;
    if (!dpf_) {
      throw std::runtime_error("TheParticipantFactoryd is null!\n");
    }
    dp_ = dpf_->create_participant(domain_id_, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dp_) {
      throw std::runtime_error("create_participant failed!\n");
    }
    configure_transport();

    // Create Publisher
    DDS::Publisher_var pub = dp_->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!pub) {
      throw std::runtime_error("create_publisher failed!\n");
    }
    // Register Type
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (!ts || ts->register_type(dp_.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error("register_type failed!\n");
    }
    // Create Topic
    DDS::Topic_var topic = dp_->create_topic("FooTopic", CORBA::String_var(ts->get_type_name()),
      TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      throw std::runtime_error("create_topic failed!\n");
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
    dw_ = pub->create_datawriter(topic.in(), qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dw_) {
      throw std::runtime_error("create_datawriter failed!\n");
    }
    OpenDDS::DCPS::DataWriterImpl* wi = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dw_.in());
    if (wi) {
      ACE_DEBUG((LM_INFO, (pfx_ + "  writer id: %C\n").c_str(), OpenDDS::DCPS::LogGuid(wi->get_repo_id()).c_str()));
    } else {
      throw std::runtime_error("dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dw_.in()) failed!\n");
    }
    writer_ = FooDataWriter::_narrow(dw_);
    if (!writer_) {
      throw std::runtime_error("FooDataWriter::_narrow failed!\n");
    }
    ACE_DEBUG((LM_INFO, (pfx_ + "->started\n").c_str()));
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception((pfx_ + " ERROR: in Publisher::Publisher").c_str());
    cleanup();
    throw;
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, ((pfx_ + " ERROR: ") + e.what()).c_str()));
    cleanup();
    throw;
  } catch (...) {
    ACE_ERROR((LM_ERROR, (pfx_ + " ERROR: exception in Publisher::Publisher\n").c_str()));
    cleanup();
    throw;
  }
}

int Publisher::publish()
{
  if (!durable_) {
    ACE_DEBUG((LM_INFO, (pfx_ + "->wait_match() before write\n").c_str()));
    Utils::wait_match(dw_, 1);
    ACE_DEBUG((LM_INFO, (pfx_ + "<-match found! before write\n").c_str()));
  }

  // Intentionally inefficient to stress various pathways related to publication:
  // write only one sample at a time per writer.
  const std::string fmt(pfx_ + "  %d%% (%d samples sent)\n");
  ProgressIndicator progress(fmt.c_str(), samples_per_thread_);
  Foo foo;
  foo.key = 3;
  foo.x = (float) thread_index_;
  DDS::InstanceHandle_t instance = writer_->register_instance(foo);
  for (std::size_t i = 0; i < samples_per_thread_; ++i) {
    foo.y = (float) i;
    if (writer_->write(foo, instance) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, (pfx_ + " ERROR: write failed!\n").c_str()));
      return 1;
    }
    ++progress;
  }

  if (durable_) {
    ACE_DEBUG((LM_INFO, (pfx_ + "->wait_match()\n").c_str()));
    Utils::wait_match(dw_, 1);
    ACE_DEBUG((LM_INFO, (pfx_ + "<-match found!\n").c_str()));
  }

  DDS::Duration_t interval = {300, 0};
  ACE_DEBUG((LM_INFO, (pfx_ + "  waiting for acks\n").c_str()));
  if (DDS::RETCODE_OK != dw_->wait_for_acknowledgments(interval)) {
    ACE_ERROR((LM_ERROR, (pfx_ + " ERROR: timed out waiting for acks!\n").c_str()));
    return 1;
  }
  ACE_DEBUG((LM_INFO, (pfx_ + "  waiting for acks returned\n").c_str()));
  return 0;
}

std::string Publisher::create_pfx(const int thread_index)
{
  std::string pfx("(%P|%t) pub");
  char i[8];
  ACE_OS::snprintf(i, 8, "%d", thread_index);
  pfx += i;
  return pfx;
}

void Publisher::cleanup()
{
  if (writer_) writer_ = 0;
  if (dw_) dw_ = 0;
  if (dp_) {
    ACE_DEBUG((LM_INFO, (pfx_ + "<-delete_contained_entities\n").c_str()));
    dp_->delete_contained_entities();
    if (dpf_) {
      ACE_DEBUG((LM_INFO, (pfx_ + "<-delete_participant\n").c_str()));
      dpf_->delete_participant(dp_.in());
      dpf_ = 0;
    }
    dp_ = 0;
  }
}

void Publisher::configure_transport()
{
  // RTPS cannot be shared
  OpenDDS::DCPS::Discovery_rch disc = TheServiceParticipant->get_discovery(domain_id_);
  OpenDDS::RTPS::RtpsDiscovery_rch rd = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(disc);
  if (!rd.is_nil()) {
    char config_name[64], inst_name[64];
    ACE_TCHAR nak_depth[8];
    ACE_OS::snprintf(config_name, 64, "cfg_%d", thread_index_);
    ACE_OS::snprintf(inst_name, 64, "rtps_%d", thread_index_);
    // The 2 is a safety factor to allow for control messages.
    ACE_OS::snprintf(nak_depth, 8, ACE_TEXT("%lu"), 2 * samples_per_thread_);
    ACE_DEBUG((LM_INFO, (pfx_ + "->transport %C\n").c_str(), config_name));
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
    TheTransportRegistry->bind_config(config_name, dp_);
  }
}
