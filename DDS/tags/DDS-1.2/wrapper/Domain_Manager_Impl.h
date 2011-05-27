// -*- C++ -*-

//=============================================================================
/**
 *  @file    Domain_Manager_Impl.h
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _DOMAIN_MANAGER_IMPL_H_
#define _DOMAIN_MANAGER_IMPL_H_

#include <ace/Refcounted_Auto_Ptr.h>
#include <ace/Null_Mutex.h>
#include "Publication_Manager.h"
#include "Subscription_Manager.h"

/// forward declaration
class Domain_Manager_Impl;

/// this defines a reference counted pointer for a domain manager
/// implementation
typedef class ACE_Refcounted_Auto_Ptr <Domain_Manager_Impl, 
				       ACE_Null_Mutex> Domain_Manager_Ptr;

/**
 * @class Domain_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all dds implementation specific domain manager classes
 */
class Domain_Manager_Impl 
{
 public:
  /// destructor
  virtual ~Domain_Manager_Impl ();

  /// this call blocks the thread until a SIGINT signal for the process is received
  virtual void run () = 0;

  /// this call causes the run method to terminate
  virtual void shutdown () = 0;

  /// getter method for the subscription manager, the caller is responsible for
  /// memory management
  virtual Subscription_Manager subscription_manager (
    const Domain_Manager_Ptr & ref) = 0;

  /// returns a subscription manager for built-in topics
  virtual Subscription_Manager builtin_topic_subscriber (
    const Domain_Manager_Ptr & ref) = 0;

  /// getter method for the publication manager, the caller is responsible for
  /// memory management
  virtual Publication_Manager publication_manager (
    const Domain_Manager_Ptr & ref) = 0;

  /// getter method for the internal domain participant
  /// the memory is managed by the Domain_Manager_Impl
  virtual DDS::DomainParticipant_ptr participant () = 0;
};

#endif /* _DOMAIN_MANAGER_IMPL_H_ */
