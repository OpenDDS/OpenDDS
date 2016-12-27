// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Domain_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_H_
#define DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_H_

#include <string>
#include <ace/Thread_Semaphore.h>
#include "Exit_Signal_Handler.h"
#include "Domain_Manager_Impl.h"

/**
 * @class OpenDDS_Domain_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief domain manager implementation for OpenDDS
 */
class OpenDDS_Domain_Manager : public Domain_Manager_Impl
{
 public:
  /// ctor
  /// will read dcps configuration and information about the used transport
  /// implementation from the command line and set up a domain participant
  /// with this.
  OpenDDS_Domain_Manager (int & argc,
                          ACE_TCHAR *argv[],
                          DDS::DomainId_t domain_id);

  /// ctor
  /// will read dcps configuration and information about the used transport
  /// implementation from the command line and set up a domain participant
  /// with this. It also allows to pass specific DomainParticipant QoS parameters
  OpenDDS_Domain_Manager (int & argc,
                          ACE_TCHAR *argv[],
                          DDS::DomainId_t domain_id,
                          const DDS::DomainParticipantQos & qos);

  virtual ~OpenDDS_Domain_Manager ();

  /// this call blocks the thread until a SIGINT signal for the process is received
  virtual void run ();

  /// this call causes the run method to terminate
  virtual void shutdown ();

  /// getter method for the subscription manager, the caller is responsible for
  /// memory management
  virtual Subscription_Manager subscription_manager (
    const Domain_Manager_Ptr & ref,
    const DDS::SubscriberQos & qos);

  /// returns a subscription manager for built-in topics
  virtual Subscription_Manager builtin_topic_subscriber (
    const Domain_Manager_Ptr & ref);

  /// getter method for the publication manager, the caller is responsible for
  /// memory management
  virtual Publication_Manager publication_manager (
    const Domain_Manager_Ptr & ref,
    const DDS::PublisherQos & qos);

  /// getter method for the internal domain participant
  /// the memory is managed by the OpenDDS_Domain_Manager
  virtual DDS::DomainParticipant_ptr participant ();

 private:
  /// processing transport implementation related options
  /// 't' - transport implementation to be used:
  ///       udp               for basic udp
  ///       multicast         for mutlicast
  ///       default_tcp       for tcp/ip
  ///       default_udp       for default udp
  ///       default_multicast for multicast with sub id
  bool parse_args (int & argc, ACE_TCHAR * argv[]);

 private:
  /// reference to the internally used domain participant
  DDS::DomainParticipant_var dp_;

  /// this semaphore is used in the run and shutdown methods
  ACE_Thread_Semaphore shutdown_lock_;

  /// signal handler for external shutdown
  Exit_Signal_Handler exit_handler_;
};

#if defined (__ACE_INLINE__)
#include "OpenDDS_Domain_Manager.inl"
#endif

#endif /* DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_H_ */
