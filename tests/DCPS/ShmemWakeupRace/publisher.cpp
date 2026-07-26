/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemWakeupRaceTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/WaitSet.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <iostream>

namespace {

bool wait_for_match(DDS::DataWriter_ptr writer)
{
  DDS::PublicationMatchedStatus match_status;
  if (writer->get_publication_matched_status(match_status) != DDS::RETCODE_OK) {
    return false;
  }
  if (match_status.current_count >= 1) {
    return true;
  }

  DDS::StatusCondition_var status_condition = writer->get_statuscondition();
  if (status_condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  if (wait_set->attach_condition(status_condition.in()) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ConditionSeq conditions;
  const DDS::Duration_t timeout = { 5, 0 };
  const DDS::ReturnCode_t wait_result = wait_set->wait(conditions, timeout);
  const DDS::ReturnCode_t detach_result = wait_set->detach_condition(status_condition.in());
  if (wait_result != DDS::RETCODE_OK || detach_result != DDS::RETCODE_OK ||
      writer->get_publication_matched_status(match_status) != DDS::RETCODE_OK) {
    return false;
  }
  return match_status.current_count >= 1;
}

}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int status = 0;
  DDS::DomainParticipantFactory_var domain_participant_factory =
    TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(ShmemWakeupRace::DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   0,
                                                   0);

  if (CORBA::is_nil(participant.in())) {
    std::cerr << "Ping: create_participant failed" << std::endl;
    status = 1;
  } else {
    ShmemWakeupRace::SampleTypeSupport_var type_support =
      new ShmemWakeupRace::SampleTypeSupportImpl;
    if (type_support->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      std::cerr << "Ping: register_type failed" << std::endl;
      status = 1;
    } else {
      CORBA::String_var type_name = type_support->get_type_name();
      DDS::Topic_var ping_topic = participant->create_topic(
        ShmemWakeupRace::PING_TOPIC_NAME, type_name.in(), TOPIC_QOS_DEFAULT, 0, 0);
      DDS::Topic_var pong_topic = participant->create_topic(
        ShmemWakeupRace::PONG_TOPIC_NAME, type_name.in(), TOPIC_QOS_DEFAULT, 0, 0);
      DDS::Publisher_var publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
      DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);
      if (CORBA::is_nil(ping_topic.in()) || CORBA::is_nil(pong_topic.in()) ||
          CORBA::is_nil(publisher.in()) || CORBA::is_nil(subscriber.in())) {
        std::cerr << "Ping: topic, publisher, or subscriber creation failed" << std::endl;
        status = 1;
      } else {
        DDS::DataWriterQos writer_qos;
        publisher->get_default_datawriter_qos(writer_qos);
        writer_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
        DDS::DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
        DDS::DataWriter_var writer = publisher->create_datawriter(ping_topic.in(), writer_qos, 0, 0);
        DDS::DataReader_var reader = subscriber->create_datareader(pong_topic.in(), reader_qos, 0, 0);
        ShmemWakeupRace::SampleDataWriter_var sample_writer =
          ShmemWakeupRace::SampleDataWriter::_narrow(writer.in());
        ShmemWakeupRace::SampleDataReader_var sample_reader =
          ShmemWakeupRace::SampleDataReader::_narrow(reader.in());
        if (CORBA::is_nil(sample_writer.in()) || CORBA::is_nil(sample_reader.in())) {
          std::cerr << "Ping: create_datawriter or create_datareader failed" << std::endl;
          status = 1;
        } else if (!wait_for_match(writer.in())) {
          std::cerr << "Ping: waiting for pong match failed" << std::endl;
          status = 1;
        } else {
          std::cout << "Ping: matched pong reader" << std::endl;
          DDS::ReadCondition_var read_condition = reader->create_readcondition(
            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
          DDS::WaitSet_var wait_set = new DDS::WaitSet;
          if (wait_set->attach_condition(read_condition.in()) != DDS::RETCODE_OK) {
            std::cerr << "Ping: attach_condition failed" << std::endl;
            status = 1;
          } else {
            ShmemWakeupRace::Sample sample;
            sample.sample_id = 1;
            if (sample_writer->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
              std::cerr << "Ping: write failed" << std::endl;
              status = 1;
            } else {
              std::cout << "Ping: wrote sample 1" << std::endl;
              DDS::ConditionSeq conditions;
              const DDS::Duration_t timeout = { 5, 0 };
              if (wait_set->wait(conditions, timeout) != DDS::RETCODE_OK) {
                std::cerr << "Ping: timed out waiting for pong" << std::endl;
                status = 1;
              } else {
                ShmemWakeupRace::SampleSeq samples;
                DDS::SampleInfoSeq sample_infos;
                const DDS::ReturnCode_t take_result = sample_reader->take(
                  samples, sample_infos, DDS::LENGTH_UNLIMITED,
                  DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
                bool received = false;
                bool return_loan_failed = false;
                if (take_result == DDS::RETCODE_OK) {
                  for (CORBA::ULong i = 0; i < samples.length(); ++i) {
                    if (sample_infos[i].valid_data && samples[i].sample_id == 1) {
                      received = true;
                      break;
                    }
                  }
                  if (sample_reader->return_loan(samples, sample_infos) != DDS::RETCODE_OK) {
                    std::cerr << "Ping: return_loan failed" << std::endl;
                    return_loan_failed = true;
                  }
                }
                if (return_loan_failed) {
                  status = 1;
                } else if (!received) {
                  std::cerr << "Ping: did not receive pong sample 1" << std::endl;
                  status = 1;
                } else {
                  std::cout << "Ping: received pong sample 1" << std::endl;
                }
              }
            }
            wait_set->detach_condition(read_condition.in());
          }
        }
      }
    }
  }

  if (!CORBA::is_nil(participant.in())) {
    participant->delete_contained_entities();
    domain_participant_factory->delete_participant(participant.in());
  }
  TheServiceParticipant->shutdown();
  return status;
}
