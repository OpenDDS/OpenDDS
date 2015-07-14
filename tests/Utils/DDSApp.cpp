/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DDSApp.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>

namespace TestUtils {

DDSApp::DDSApp(int& argc, ACE_TCHAR**& argv)
// default id to 0, but still allow it to be set
: domain_id_defaulted_(false)
, default_domain_id_(0)
, dpf_(TheParticipantFactoryWithArgs(argc, argv))
, shutdown_(false)
{
}

DDSApp::DDSApp(int& argc,
               ACE_TCHAR**& argv,
               DDS::DomainId_t default_domain_id)
: domain_id_defaulted_(true)
, default_domain_id_(default_domain_id)
, dpf_(TheParticipantFactoryWithArgs(argc, argv))
, shutdown_(false)
{
}

DDSApp::~DDSApp()
{
  shutdown();
}

DDS::DomainParticipant_var
DDSApp::participant()
{
  return create_part(default_domain_id_,
                     PARTICIPANT_QOS_DEFAULT,
                     DDS::DomainParticipantListener::_nil(),
                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

DDS::DomainParticipant_var
DDSApp::participant(DDS::DomainId_t domain_id)
{
  return create_part(domain_id,
                     PARTICIPANT_QOS_DEFAULT,
                     DDS::DomainParticipantListener::_nil(),
                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

DDS::DomainParticipant_var
DDSApp::participant(DDS::DomainId_t                    domain_id,
                    DDS::DomainParticipantListener_var listener,
                    DDS::StatusMask                    mask)
{
  return create_part(domain_id,
                     PARTICIPANT_QOS_DEFAULT,
                     listener,
                     mask);
}

DDS::DomainParticipant_var
DDSApp::create_part(DDS::DomainId_t                    domain_id,
                    const DDS::DomainParticipantQos&   qos,
                    DDS::DomainParticipantListener_var listener,
                    DDS::StatusMask                    mask)
{
  DDS::DomainParticipant_var participant =
    dpf_->create_participant(domain_id,
                             qos,
                             listener.in(),
                             mask);
  add(participant);
  return participant;
}

DDS::Publisher_var
DDSApp::publisher(DDS::DomainParticipant_var participant,
                  DDS::PublisherListener_var a_listener,
                  DDS::StatusMask            mask)
{
  return create_pub(participant,
                    PUBLISHER_QOS_DEFAULT,
                    a_listener,
                    mask);
}

DDS::Publisher_var
DDSApp::create_pub(DDS::DomainParticipant_var participant,
                   const DDS::PublisherQos&   qos,
                   DDS::PublisherListener_var a_listener,
                   DDS::StatusMask            mask)
{
  determine_participant(participant);
  return participant->create_publisher(qos,
                                       a_listener.in(),
                                       mask);
}

DDS::Subscriber_var
DDSApp::subscriber(DDS::DomainParticipant_var  participant,
                   DDS::SubscriberListener_var a_listener,
                   DDS::StatusMask             mask)
{
  return create_sub(participant,
                    SUBSCRIBER_QOS_DEFAULT,
                    a_listener,
                    mask);
}

DDS::Subscriber_var
DDSApp::create_sub(DDS::DomainParticipant_var  participant,
                   const DDS::SubscriberQos&   qos,
                   DDS::SubscriberListener_var a_listener,
                   DDS::StatusMask             mask)
{
  determine_participant(participant);
  return participant->create_subscriber(qos,
                                        a_listener.in(),
                                        mask);
}

// either cleans up the specified participant, or all participants and the domain participant factory
void
DDSApp::cleanup(DDS::DomainParticipant_var participant)
{
  determine_participant(participant);
  participant->delete_contained_entities();
  dpf_->delete_participant(participant.in());

  remove(participant);
}

void
DDSApp::add(const DDS::DomainParticipant_var& participant)
{
  participants_.insert(std::make_pair(participant.in(), participant));
  if (!default_participant_.in())
    default_participant_ = participant;
}

void
DDSApp::remove(const DDS::DomainParticipant_var& participant)
{
  participants_.erase(participant.in());
  if (participant.in() == default_participant_.in()) {
    if (participants_.empty()) {
      default_participant_ = DDS::DomainParticipant_var();
    }
    else {
      ACE_DEBUG((LM_DEBUG,
                 "NOTE: removed default participant and assigning new "
                 "default\n"));
      default_participant_ = participants_.begin()->second;
    }
  }
}

void
DDSApp::determine_participant(DDS::DomainParticipant_var& part)
{
  if (part.in())
    return;

  if (!default_participant_.in()) {
    // this will create a participant and assign the default participant
    participant();
  }

  part = default_participant_;
}

void
DDSApp::assign_default_domain_id(DDS::DomainId_t id)
{
  if (!domain_id_defaulted_) {
    domain_id_defaulted_ = true;
    default_domain_id_ = id;
  }
}

void
DDSApp::shutdown()
{
  if (shutdown_)
    return;

  shutdown_ = true;

  for (Participants::const_iterator part = participants_.begin();
       part != participants_.end();
       ++part) {
    part->second->delete_contained_entities();
    dpf_->delete_participant(part->first);
  }

  participants_.clear();
  TheServiceParticipant->shutdown();
}

} // End namespaces
