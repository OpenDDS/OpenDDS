// -*- C++ -*-

//=============================================================================
/**
 *  @file    Domain_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_DOMAIN_MANAGER_H_
#define DDS_WRAPPER_DOMAIN_MANAGER_H_

#include "DDSWrapper_export.h"
#include "Domain_Manager_Impl.h"

/**
 * @class ManagerException
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief general purpose exception class for the manager classes
 */
class DDSWrapper_Export Manager_Exception {
 public:
  /// constructor
  Manager_Exception (const std::string& reason);

  /// getter method for the reason_ class member
  std::string reason () const;

 private:
  std::string reason_; /// description of the reason for the exception
};

/**
 * @class Domain_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief class for memory management of Domain_Manager_Impl classes
 *
 * This class plays the role of an Abstraction in the Bridge pattern.
 */
class DDSWrapper_Export Domain_Manager
{
 public:
  /// default ctor
  Domain_Manager ();

  /// ctor
  /// @param argc number of command line arguments
  /// @param argv commandline arguments used for initialization
  /// @param domain_id in which domain participant should be registered
  Domain_Manager (int & argc,
                  ACE_TCHAR *argv[],
                  DDS::DomainId_t domain_id);

  /// ctor
  /// @param argc number of command line arguments
  /// @param argv commandline arguments used for initialization
  /// @param domain_id in which domain participant should be registered
  /// @param qos for the domain participant
  Domain_Manager (int & argc,
                  ACE_TCHAR *argv[],
                  DDS::DomainId_t domain_id,
                  const DDS::DomainParticipantQos & qos);

  /// ctor that takes ownership of the passed in impl pointer
  Domain_Manager (Domain_Manager_Ptr impl);

  /// copy constructor
  Domain_Manager (const Domain_Manager & copy);

  /// assignment operator
  void operator= (const Domain_Manager& copy);

  /// checks for null reference
  bool null () const;

  /// this call blocks the thread until a SIGINT signal for the process is received
  void run ();

  /// this call causes the run method to terminate
  void shutdown ();

  /// factory method for subscription managers
  Subscription_Manager subscription_manager (const DDS::SubscriberQos & qos
                                               = SUBSCRIBER_QOS_DEFAULT);

  /// returns a subscription manager for built-in topics
  Subscription_Manager builtin_topic_subscriber ();

  /// getter method for publication managers
  Publication_Manager publication_manager (const DDS::PublisherQos & qos
                                             = PUBLISHER_QOS_DEFAULT);

  /// getter method for the internal domain participant
  /// the memory is managed by the Domain_Manager
  DDS::DomainParticipant_ptr participant ();

 private:
  /// reference counted auto pointer containing the impl pointer
  Domain_Manager_Ptr manager_impl_;
};

#if defined (__ACE_INLINE__)
#include "Domain_Manager.inl"
#endif

#endif /* DDS_WRAPPER_DOMAIN_MANAGER_H_ */
