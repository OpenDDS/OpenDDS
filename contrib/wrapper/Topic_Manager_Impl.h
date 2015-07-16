// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager_Impl.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_TOPIC_MANAGER_IMPL_H_
#define DDS_WRAPPER_TOPIC_MANAGER_IMPL_H_

#include <string>
#include <ace/Refcounted_Auto_Ptr.h>
#include <ace/Null_Mutex.h>
#include "DDSWrapper_export.h"
#include "wrapper_publication.h"
#include "wrapper_subscription.h"


/// forward declarations
class Domain_Manager;
class Subscription_Manager;
class Publication_Manager;
class Topic_Manager_Impl;

/// this defines a reference counted pointer for a topic manager
/// implementation
typedef class ACE_Refcounted_Auto_Ptr <Topic_Manager_Impl,
                                       ACE_Null_Mutex> Topic_Manager_Ptr;

/**
 * @class Topic_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief abstract interface for topic creation and usage
 */
class DDSWrapper_Export Topic_Manager_Impl
{
 public:
  /// dtor
  virtual ~Topic_Manager_Impl ();

  /// getter method for the topic name
  virtual std::string name () const = 0;

  /// this method uses the dm participant to create a topic
  virtual void create_topic (Domain_Manager & dm) = 0;

  /// this method uses the dm participant to create a topic
  virtual void delete_topic (Domain_Manager & dm) = 0;

  /// this method returns a new datareader created by a subscriber
  /// memory management has to be done by the caller
  virtual DDS::DataReader_ptr datareader (const Subscription_Manager & sm,
                                          const DDS::DataReaderQos & qos) = 0;

  /// this method returns a new datawriter created by a publisher
  /// memory management has to be done by the caller
  virtual DDS::DataWriter_ptr datawriter (const Publication_Manager & pm,
                                          const DDS::DataWriterQos & qos) = 0;
};

#endif /* DDS_WRAPPER_TOPIC_MANAGER_IMPL_H_ */
