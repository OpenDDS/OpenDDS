// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Domain_Manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "OpenSplice_Domain_Manager.h"

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/Arg_Shifter.h>
#include <ace/Sig_Handler.h>
#include <ccpp_dds_dcps.h>
#include "Domain_Manager.h"
#include "Subscription_Manager.h"
#include "Publication_Manager.h"
#include "OpenSplice_Subscription_Manager.h"
#include "OpenSplice_Publication_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenSplice_Domain_Manager.inl"
#endif

OpenSplice_Domain_Manager::OpenSplice_Domain_Manager (int & argc, 
				char* argv[], 
				DDS::DomainId_t domain_id)
  : dp_ (DDS::DomainParticipant::_nil ()),
    shutdown_lock_ (0),
    exit_handler_ (shutdown_lock_)
{
  // get the domain participant factory
  DDS::DomainParticipantFactory_var dpf =
    DDS::DomainParticipantFactory::get_instance ();

  if (CORBA::is_nil (dpf.in ()))
    throw Manager_Exception ("OpenSplice_Domain_Manager ctor could not get "
			     "instance to DomainParticipant factory.");

  if (!this->parse_args (argc, argv))
    throw Manager_Exception ("OpenSplice_Domain_Manager ctor could process "
			     "command line arguments.");

  // create the participant named 'participant'.
  dp_ = dpf->create_participant (domain_id,
				 PARTICIPANT_QOS_DEFAULT,
				 DDS::DomainParticipantListener::_nil (),
				 DDS::ANY_STATUS);

  // check for successful creation
  if (CORBA::is_nil (dp_.in ()))
    throw Manager_Exception ("OpenSplice_Domain_Manager ctor failed to create "
			     "domain participant.");

  // add a the handler for the SIGINT signal here
  ACE_Sig_Handler sig_handler;
  sig_handler.register_handler (SIGINT, &exit_handler_);
}

OpenSplice_Domain_Manager::~OpenSplice_Domain_Manager ()
{
  // delete the participant's contained entities
  if (!CORBA::is_nil (dp_.in ()))
    dp_->delete_contained_entities ();

  // get the domain participant factory
  DDS::DomainParticipantFactory_var dpf =
    DDS::DomainParticipantFactory::get_instance ();

  if (!CORBA::is_nil (dpf.in ())) 
    dpf->delete_participant(dp_.in ());
}

void
OpenSplice_Domain_Manager::run ()
{
  // aquire the lock and block
  ACE_Guard <ACE_Thread_Semaphore> guard (shutdown_lock_);
}

void
OpenSplice_Domain_Manager::shutdown ()
{
  // releasing the lock makes the run method terminate
  shutdown_lock_.release ();
}

Subscription_Manager
OpenSplice_Domain_Manager::subscription_manager (
  const Domain_Manager_Ptr & ref,
  const DDS::SubscriberQos & qos)
{
  // create new subscription manager
  return Subscription_Manager (
           Subscription_Manager_Ptr (
             new OpenSplice_Subscription_Manager (Domain_Manager (ref), qos)));
}

Subscription_Manager
OpenSplice_Domain_Manager::builtin_topic_subscriber (
  const Domain_Manager_Ptr & ref)
{
  return Subscription_Manager (
           Subscription_Manager_Ptr (
             new OpenSplice_Subscription_Manager (
               Domain_Manager (ref), 
	       dp_->get_builtin_subscriber ())));
}

Publication_Manager
OpenSplice_Domain_Manager::publication_manager (
  const Domain_Manager_Ptr & ref,
  const DDS::PublisherQos & qos)
{
  // create new publication manager
  return Publication_Manager (
           Publication_Manager_Ptr (
             new OpenSplice_Publication_Manager (Domain_Manager (ref), 
                                                 qos)));
}

bool
OpenSplice_Domain_Manager::parse_args (int & /* argc */, char * /* argv */[])
{
  /* there is no parameter prosessing right now
  ACE_Arg_Shifter arg_shifter (argc, argv);

  const char *current = 0;
  
  // Ignore the command - argv[0].
  arg_shifter.ignore_arg ();
  
  while (arg_shifter.is_anything_left ()) 
    {
      if ((current = arg_shifter.get_the_parameter ("-t")) != 0) 
        {
          arg_shifter.consume_arg ();
	  return false
        }
      else
        {
          arg_shifter.ignore_arg ();
        }
    }
  */
  return true;  
}
