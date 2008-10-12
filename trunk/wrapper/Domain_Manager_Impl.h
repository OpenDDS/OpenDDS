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

#include "Publication_Manager.h"
#include "Subscription_Manager.h"
#include "Reference_Counter_T.h"

/**
 * @class Domain_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all dds implementation specific domain manager classes
 */
class Domain_Manager_Impl 
{
  /// this friend declaration is needed for reference counting purposes
  friend class Reference_Counter_T <Domain_Manager_Impl>;

 public:
  /// ctor
  /// will read dcps configuration and information about the used transport 
  /// implementation from the command line and set up a domain participant 
  /// with this.
  Domain_Manager_Impl ();

  /// destructor
  virtual ~Domain_Manager_Impl ();

  /// this call blocks the thread until a SIGINT signal for the process is received
  virtual void run () = 0;

  /// this call causes the run method to terminate
  virtual void shutdown () = 0;

  /// getter method for the subscription manager, the caller is responsible for
  /// memory management
  virtual Subscription_Manager subscription_manager () = 0;

  /// returns a subscription manager for built-in topics
  virtual Subscription_Manager builtin_topic_subscriber () = 0;

  /// getter method for the publication manager, the caller is responsible for
  /// memory management
  virtual Publication_Manager publication_manager () = 0;

  /// getter method for the internal domain participant
  /// the memory is managed by the Domain_Manager_Impl
  virtual DDS::DomainParticipant_ptr participant () = 0;

 protected:
  /// reference count variable
  unsigned long use_;
};

#endif /* _DOMAIN_MANAGER_IMPL_H_ */
