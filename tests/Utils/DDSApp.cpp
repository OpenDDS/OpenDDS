/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DDSApp.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>

namespace {
  ACE_TCHAR** copy_argv(int argc, ACE_TCHAR* argv[])
  {
    ACE_TCHAR** new_argv = new ACE_TCHAR*[argc];
    for (int i = 0; i < argc; ++i) {
      new_argv[i] = argv[i];
    }

    return new_argv;
  }
}

namespace TestUtils {

DDSApp::DDSApp(int argc, ACE_TCHAR* argv[])
: argc_(argc)
, argv_(argv)//copy_argv(argc, argv))
// default id to 0, but still allow it to be set
, domain_id_defaulted_(false)
, default_domain_id_(0)
, shutdown_(false)
{
}

DDSApp::DDSApp(int argc,
               ACE_TCHAR* argv[],
               ::DDS::DomainId_t default_domain_id)
: argc_(argc)
, argv_(copy_argv(argc, argv))
, domain_id_defaulted_(true)
, default_domain_id_(default_domain_id)
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
  return participant(default_domain_id_);
}

DDS::DomainParticipant_var
DDSApp::participant(::DDS::DomainId_t domain_id)
{
  DDS::DomainParticipantFactory_var dpf = domain_participant_factory();
  DDS::DomainParticipant_var participant =
    dpf->create_participant(domain_id,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  add(participant);
  return participant;
}

DDS::Publisher_var
DDSApp::publisher(
  DDS::DomainParticipant_var participant)
{
  determine_participant(participant);
  return participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

DDS::Subscriber_var
DDSApp::subscriber(
  DDS::DomainParticipant_var participant)
{
  determine_participant(participant);
  return participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

// either cleans up the specified participant, or all participants and the domain participant factory
void
DDSApp::cleanup(DDS::DomainParticipant_var participant)
{
  determine_participant(participant);
  participant->delete_contained_entities();
  domain_participant_factory()->delete_participant(participant.in());

  remove(participant);
}

DDS::DomainParticipantFactory_var
DDSApp::domain_participant_factory()
{
  // only use dpf_dont_use_ in this method
  if (!dpf_dont_use_.in())
    // Initialize DomainParticipantFactory
    dpf_dont_use_ = TheParticipantFactoryWithArgs(argc_, argv_);

  return dpf_dont_use_;
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
  if (participant == default_participant_) {
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
DDSApp::assign_default_domain_id(::DDS::DomainId_t id)
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

  const DDS::DomainParticipantFactory_var dpf = domain_participant_factory();
  for (Participants::const_iterator part = participants_.begin();
       part != participants_.end();
       ++part) {
    part->second->delete_contained_entities();
    dpf->delete_participant(part->first);
  }

  participants_.clear();
  TheServiceParticipant->shutdown();
}

} // End namespaces
