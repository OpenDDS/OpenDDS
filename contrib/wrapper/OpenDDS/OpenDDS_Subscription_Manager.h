// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Subscription_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_H_
#define DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_H_

#include "Subscription_Manager_Impl.h"
#include "Domain_Manager.h"

/**
 * @class OpenDDS_Subscription_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief subscription manager implementation for OpenDDS
 *
 * This class keeps a DDS subscriber to create new topics
 * for a topic manager.
 */
class OpenDDS_Subscription_Manager : public Subscription_Manager_Impl
{
 public:
  /// ctor with transport impl registration
  OpenDDS_Subscription_Manager (const Domain_Manager & dm,
                                const DDS::SubscriberQos & qos);

  /// ctor
  /// will take control of an existing subscriber
  OpenDDS_Subscription_Manager (const Domain_Manager & dm,
                                DDS::Subscriber_ptr sub);

  /// dtor
  virtual ~OpenDDS_Subscription_Manager ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  virtual void access_topic (
    const Topic_Manager & topic,
    const DDS::DataReaderQos & qos,
    const Subscription_Manager_Ptr & ref);

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic);

  /// returns a data reader for a specific topic
  virtual DDS::DataReader_ptr lookup_datareader (const std::string & topic_name);

  /// creates and returns qos for data readers with the default values
  virtual DDS::DataReaderQos get_default_datareader_qos ();

  /// returns the underlying subsriber instance
  /// memory management of the returned subscriber reference is done by the
  /// OpenDDS_Subscription_Manager itself
  virtual DDS::Subscriber_ptr subscriber () const;

 private:
  /// initializes the publication manager
  void init (const DDS::SubscriberQos & qos);

  /// reference to the domain manager
  Domain_Manager dm_;

  /// reference to the internally used subscriber
  DDS::Subscriber_var sub_;
};

#if defined (__ACE_INLINE__)
#include "OpenDDS_Subscription_Manager.inl"
#endif

#endif /* DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_H_ */
