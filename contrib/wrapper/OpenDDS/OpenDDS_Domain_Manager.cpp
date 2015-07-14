// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Domain_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "OpenDDS_Domain_Manager.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/Arg_Shifter.h>
#include "Domain_Manager.h"
#include "Subscription_Manager.h"
#include "Publication_Manager.h"
#include "OpenDDS_Subscription_Manager.h"
#include "OpenDDS_Publication_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenDDS_Domain_Manager.inl"
#endif

OpenDDS_Domain_Manager::OpenDDS_Domain_Manager (int & argc,
                                ACE_TCHAR* argv[],
                                DDS::DomainId_t domain_id)
  : dp_ (DDS::DomainParticipant::_nil ()),
    shutdown_lock_ (0),
    exit_handler_ (shutdown_lock_)
{
  // get the domain participant factory from the singleton
  DDS::DomainParticipantFactory_var dpf =
    OpenDDS::DCPS::Service_Participant::instance ()->
      get_domain_participant_factory (argc, argv);

  this->parse_args (argc, argv);

  // create the participant named 'participant'.
  dp_ = dpf->create_participant (domain_id,
                                 PARTICIPANT_QOS_DEFAULT,
                                 DDS::DomainParticipantListener::_nil (),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // check for successful creation
  if (CORBA::is_nil (dp_.in ()))
    throw Manager_Exception ("Failed to create domain participant.");

  // add a the handler for the SIGINT signal here
  ACE_Sig_Handler sig_handler;
  sig_handler.register_handler (SIGINT, &exit_handler_);
}

OpenDDS_Domain_Manager::OpenDDS_Domain_Manager (int & argc,
                                ACE_TCHAR* argv[],
                                DDS::DomainId_t domain_id,
                                const DDS::DomainParticipantQos & qos)
  : dp_ (DDS::DomainParticipant::_nil ()),
    shutdown_lock_ (0),
    exit_handler_ (shutdown_lock_)
{
  // get the domain participant factory from the singleton
  DDS::DomainParticipantFactory_var dpf =
    OpenDDS::DCPS::Service_Participant::instance ()->
      get_domain_participant_factory (argc, argv);

  this->parse_args (argc, argv);

  // create the participant named 'participant'.
  dp_ = dpf->create_participant (domain_id,
                                 qos,
                                 DDS::DomainParticipantListener::_nil (),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // check for successful creation
  if (CORBA::is_nil (dp_.in ()))
    throw Manager_Exception ("Failed to create domain participant.");

  // add a the handler for the SIGINT signal here
  ACE_Sig_Handler sig_handler;
  sig_handler.register_handler (SIGINT, &exit_handler_);
}

OpenDDS_Domain_Manager::~OpenDDS_Domain_Manager ()
{
  // delete the participant's contained entities
  if (!CORBA::is_nil (dp_.in ()))
    dp_->delete_contained_entities ();

  DDS::DomainParticipantFactory_var dpf =
    OpenDDS::DCPS::Service_Participant::instance()->
    get_domain_participant_factory ();

  if (!CORBA::is_nil (dpf.in ()))
    dpf->delete_participant(dp_.in ());

  // shut down the service participant
  OpenDDS::DCPS::Service_Participant::instance ()->shutdown ();
}

void
OpenDDS_Domain_Manager::run ()
{
  // aquire the lock and block
  ACE_Guard <ACE_Thread_Semaphore> guard (shutdown_lock_);
}

void
OpenDDS_Domain_Manager::shutdown ()
{
  // releasing the lock makes the run method terminate
  shutdown_lock_.release ();
}

Subscription_Manager
OpenDDS_Domain_Manager::subscription_manager (
  const Domain_Manager_Ptr & ref,
  const DDS::SubscriberQos & qos)
{
  // use the simple constructor for consecutive calls of this method
  return Subscription_Manager (
    Subscription_Manager_Ptr (
      new OpenDDS_Subscription_Manager (Domain_Manager (ref),
                                        qos)));
}

Subscription_Manager
OpenDDS_Domain_Manager::builtin_topic_subscriber (const Domain_Manager_Ptr & ref)
{
  return Subscription_Manager (
           Subscription_Manager_Ptr (
             new OpenDDS_Subscription_Manager (
               Domain_Manager (ref),
               dp_->get_builtin_subscriber ())));
}

Publication_Manager
OpenDDS_Domain_Manager::publication_manager (const Domain_Manager_Ptr & ref,
                                             const DDS::PublisherQos & qos)
{
  // use the simple constructor for consecutive calls of this method
  return Publication_Manager (
    Publication_Manager_Ptr (
      new OpenDDS_Publication_Manager (Domain_Manager (ref),
                                       qos)));
}

bool
OpenDDS_Domain_Manager::parse_args (int & argc, ACE_TCHAR * argv [])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  const ACE_TCHAR *current = 0;

  // Ignore the command - argv[0].
  arg_shifter.ignore_arg ();

  int transport_impl_id_; // transport switching not implemented for OpenDDS 3.x

  while (arg_shifter.is_anything_left ())
    {
      if ((current = arg_shifter.get_the_parameter (ACE_TEXT("-t"))) != 0)
        {
            if (ACE_OS::strcmp (current, ACE_TEXT("udp")) == 0)
              {
                transport_impl_id_ = 2;
              }
            else if (ACE_OS::strcmp (current,
                                     ACE_TEXT("multicast")) == 0)
              {
                transport_impl_id_ = 3;
              }
            else if (ACE_OS::strcmp (current,
                                     ACE_TEXT("default_tcp")) == 0)
              {
                transport_impl_id_ = 4; //OpenDDS::DCPS::DEFAULT_TCP_ID;
              }
            else if (ACE_OS::strcmp (current,
                                     ACE_TEXT("default_udp")) == 0)
              {
                transport_impl_id_ = 5; //OpenDDS::DCPS::DEFAULT_UDP_ID;
              }
            else if (ACE_OS::strcmp (current,
                                     ACE_TEXT("default_multicast")) == 0)
              {
                transport_impl_id_ = 6; //OpenDDS::DCPS::DEFAULT_MULTICAST_ID;
              }
            else
              {
                ACE_DEBUG ((LM_ERROR,
                            ACE_TEXT ("Unkown value %s for -t option.\n"),
                            current));
                return false;
              }

          arg_shifter.consume_arg ();
        }
      else
        {
          arg_shifter.ignore_arg ();
        }
    }

  return true;
}
