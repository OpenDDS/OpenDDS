// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager_Impl.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_SUBSCRIPTION_MANAGER_IMPL_H_
#define DDS_WRAPPER_SUBSCRIPTION_MANAGER_IMPL_H_

#include <string>
#include <ace/Refcounted_Auto_Ptr.h>
#include <ace/Null_Mutex.h>
#include "wrapper_subscription.h"

/// forward declarations
class Topic_Manager;
class Subscription_Manager_Impl;

/// this defines a reference counted pointer for a subscription manager
/// implementation
typedef class ACE_Refcounted_Auto_Ptr <Subscription_Manager_Impl,
                                       ACE_Null_Mutex> Subscription_Manager_Ptr;

/**
 * @class Subscription_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all subscription manager implementations
 */
class Subscription_Manager_Impl
{
 public:
  virtual ~Subscription_Manager_Impl ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  virtual void access_topic (
    const Topic_Manager & topic,
    const DDS::DataReaderQos & qos,
    const Subscription_Manager_Ptr & ref) = 0;

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic) = 0;

  /// returns a data reader for a specific topic
  virtual DDS::DataReader_ptr lookup_datareader (
                                const std::string & topic_name) = 0;

  /// creates and returns qos for data readers with the default values
  virtual DDS::DataReaderQos get_default_datareader_qos () = 0;

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the
  /// Subscription_Manager_Impl itself
  virtual DDS::Subscriber_ptr subscriber () const = 0;
};

#endif /* DDS_WRAPPER_SUBSCRIPTION_MANAGER_IMPL_H_ */
