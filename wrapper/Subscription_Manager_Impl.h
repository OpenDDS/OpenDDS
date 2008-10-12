// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager_Impl.h
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _SUBSCRIPTION_MANAGER_IMPL_H_
#define _SUBSCRIPTION_MANAGER_IMPL_H_

#include <string>
#include <dds/DdsDcpsSubscriptionC.h>
#include "Reference_Counter_T.h"

/// forward declaration
class Topic_Manager;

/**
 * @class Subscription_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all subscription manager implementations
 */
class Subscription_Manager_Impl 
{
  /// this friend declaration is needed for reference counting purposes
  friend class Reference_Counter_T <Subscription_Manager_Impl>;

 public:
  // default ctor
  Subscription_Manager_Impl ();

  /// destructor
  virtual ~Subscription_Manager_Impl ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  virtual void access_topic (const Topic_Manager & topic) = 0;

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic) = 0;

  /// returns a data reader for a specific topic
  virtual DDS::DataReader_ptr lookup_datareader (
                                const std::string & topic_name) = 0;

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the 
  /// Subscription_Manager_Impl itself
  virtual DDS::Subscriber_ptr subscriber () const = 0;

 protected:
  /// reference count variable
  unsigned long use_;
};

#endif /* _SUBSCRIPTION_MANAGER_IMPL_H_ */
