// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_SUBSCRIPTION_MANAGER_H_
#define DDS_WRAPPER_SUBSCRIPTION_MANAGER_H_

#include "DDSWrapper_export.h"
#include "Subscription_Manager_Impl.h"

/**
 * @class Subscription_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief class for memory management of a Subscription_ManagerImpl class
 *
 * This class plays the role of an Abstraction in the Bridge pattern.
 */
class DDSWrapper_Export Subscription_Manager
{
 public:
  /// default ctor
  Subscription_Manager ();

  /// ctor that takes ownership of the passed in impl pointer
  Subscription_Manager (Subscription_Manager_Ptr impl);

  /// copy constructor
  Subscription_Manager (const Subscription_Manager & copy);

  /// assignment operator
  void operator= (const Subscription_Manager & copy);

  /// checks for null reference
  bool null () const;

  /// will create a topic instance using the domain manager
  /// internally a datareader will be created which can be accessed
  /// through the lookup_datareader method
  void access_topic (const Topic_Manager & topic,
                     const DDS::DataReaderQos & qos
                       = DATAREADER_QOS_DEFAULT);

  /// unregisters and deletes the topic from the domain
  void remove_topic (const Topic_Manager & topic);

  /// creates and returns qos for data readers with the default values
  DDS::DataReaderQos get_default_datareader_qos ();

  /// returns a data reader for a specific topic
  DDS::DataReader_ptr lookup_datareader (const std::string & topic_name);

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the
  /// Subscription_Manager itself
  DDS::Subscriber_ptr subscriber () const;

 private:
  /// reference counted auto pointer containing the impl pointer
  Subscription_Manager_Ptr manager_impl_;
};

#if defined (__ACE_INLINE__)
#include "Subscription_Manager.inl"
#endif

#endif /* DDS_WRAPPER_SUBSCRIPTION_MANAGER_H_ */
