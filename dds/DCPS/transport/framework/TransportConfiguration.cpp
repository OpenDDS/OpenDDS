// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfiguration.h"
#include "ThreadSynchStrategy.h"
#include "EntryExit.h"

#include "ace/Sched_Params.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfiguration.inl"
#endif /* ! __ACE_INLINE__ */


OpenDDS::DCPS::TransportConfiguration::~TransportConfiguration()
{
  DBG_ENTRY_LVL("TransportConfiguration","~TransportConfiguration",6);
  delete this->send_thread_strategy_;
}

int
OpenDDS::DCPS::TransportConfiguration::load (const TransportIdType& id
                                             , ACE_Configuration_Heap& cf)
{
  ACE_TCHAR section [50];
  ACE_OS::sprintf (section, ACE_TEXT("%s%u")
                   , ACE_TEXT_ALWAYS_CHAR(TRANSPORT_SECTION_NAME_PREFIX), id);
  const ACE_Configuration_Section_Key &root = cf.root_section ();
  ACE_Configuration_Section_Key sect;
  if (cf.open_section (root, section, 0, sect) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Failed to open section: %s\n"), section),
                       -1);
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("swap_bytes"), this->swap_bytes_, bool)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("queue_messages_per_pool"), this->queue_messages_per_pool_, size_t)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("queue_initial_pools"), this->queue_initial_pools_, size_t)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("max_packet_size"), this->max_packet_size_, ACE_UINT32)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("max_samples_per_packet"), this->max_samples_per_packet_, size_t)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("optimum_packet_size"), this->optimum_packet_size_, ACE_UINT32)
  GET_CONFIG_STRING_VALUE (cf, sect, ACE_TEXT("transport_type"), this->transport_type_)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("thread_per_connection"), this->thread_per_connection_, bool)
  GET_CONFIG_VALUE (cf, sect, ACE_TEXT("datalink_release_delay"), this->datalink_release_delay_, int)

  //
  // Establish the scheduler if specified.
  //
  ACE_TString schedulerString;
  if( 0 == cf.get_string_value( sect, ACE_TEXT("scheduler"), schedulerString)) {
    //
    // Translate the scheduling policy to a usable value.
    //
    this->scheduler_ = ACE_SCHED_OTHER;
    if( schedulerString == "SCHED_RR") {
      this->scheduler_ = ACE_SCHED_RR;

    } else if( schedulerString == "SCHED_FIFO") {
      this->scheduler_ = ACE_SCHED_FIFO;

    } else if( schedulerString == "SCHED_OTHER") {
      this->scheduler_ = ACE_SCHED_OTHER;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: TransportConfiguration::load() - ")
        ACE_TEXT("unrecognized scheduling policy: %s, set to SCHED_OTHER.\n"),
        schedulerString.c_str()
      ));

    }

    //
    // Attempt to set the scheduling policy.
    //
    ACE_Sched_Params sched_params(
                       this->scheduler_,
                       ACE_DEFAULT_THREAD_PRIORITY,
                       ACE_SCOPE_PROCESS
                     );
    if( ACE_OS::sched_params(
          ACE_Sched_Params( this->scheduler_,
            ACE_Sched_Params::priority_min(this->scheduler_),
            ACE_SCOPE_PROCESS
      )) != 0) {
      if( ACE_OS::last_error() == EPERM) {
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: TransportConfiguration::load() - ")
          ACE_TEXT("user is not superuser, scheduler not set.\n")
        ));

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: TransportConfiguration::load() - ")
          ACE_TEXT("sched_params failed: %m.\n")
        ));
      }

    } else if( DCPS_debug_level > 0) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) TransportConfiguration::load() - ")
        ACE_TEXT("scheduling policy set to %s(%d).\n"),
        schedulerString.c_str()
      ));
    }
  }

  adjust_config_value ();
  return 0;
}
