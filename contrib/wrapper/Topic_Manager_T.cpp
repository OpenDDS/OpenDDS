// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager_T.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_TOPIC_MANAGER_T_CPP_
#define DDS_WRAPPER_TOPIC_MANAGER_T_CPP_

#include <memory>
#include <ace/streams.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsTopicC.h>
#include <dds/DdsDcpsDomainC.h>

#include "Domain_Manager.h"
#include "Subscription_Manager.h"
#include "Publication_Manager.h"

template <typename TYPE_SUPPORT, typename TS_IMPL>
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::Topic_Manager_T (
    const std::string & name,
    DDS::DataReaderListener_ptr listener,
    bool create_new_topic)
  : name_ (name),
    topic_ (DDS::Topic::_nil ()),
    type_ (new TS_IMPL ()),
    listener_ (DDS::DataReaderListener::_duplicate (listener)),
    create_new_topic_ (create_new_topic)
{
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::Topic_Manager_T (
    const Topic_Manager_T & orig)
  : Topic_Manager_Impl (),
    name_ (orig.name_),
    topic_ (DDS::Topic::_duplicate (orig.topic_.in ())),
    type_ (TYPE_SUPPORT::_duplicate (orig.type_)),
    listener_ (DDS::DataReaderListener::_duplicate (orig.listener_))
{
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
void
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::operator= (
  const Topic_Manager_T & rhs)
{
  // check for self assignment
  if (&rhs != this)
    {
      name_ = rhs.name_;
      topic_ = DDS::Topic::_duplicate (rhs.topic_.in ());
      type_ = TYPE_SUPPORT::_duplicate (rhs.type_);
      listener_ = DDS::DataReaderListener::_duplicate (rhs.listener_);
    }
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::~Topic_Manager_T ()
{
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
std::string
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::name () const
{
  return name_;
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
void
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::create_topic (Domain_Manager & dm)
{
  // the corresponding type has to be registered with the participant
  // before a topic can be created
  // the "" is necessary because there is no default value for this
  // parameter and no version with just one parameter
  if (type_->register_type (dm.participant (), 0)
      != DDS::RETCODE_OK)
    throw Manager_Exception ("Failed to register the type support for Quoter.");

  if (create_new_topic_)
    {
      // get type name and create a bew topic of this type.
      CORBA::String_var type_name = type_->get_type_name ();

      topic_ =
        dm.participant ()->create_topic (name_.c_str (),
                                         type_name.in (),
                                         TOPIC_QOS_DEFAULT,
                                         ::DDS::TopicListener::_nil (),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    }
  else
    {
      DDS::Duration_t timeout = {1, 0};

      topic_ = dm.participant ()->find_topic (name_.c_str (),
                                                timeout);
    }

  // check if topic creation was successful
  if (CORBA::is_nil (topic_.in ()))
    {
      std::string error_msg ("Failed to create topic ");
      error_msg += name_.c_str ();

      throw Manager_Exception (error_msg);
    }
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
void
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::delete_topic (Domain_Manager & dm)
{
  if (dm.participant ()->delete_topic (topic_.in ())
      != DDS::RETCODE_OK)
    {
      throw Manager_Exception ("Could not delete topic.");
    }
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
DDS::DataReader_ptr
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::datareader (const Subscription_Manager & sm,
                                                     const DDS::DataReaderQos & qos)
{
  // check if subscriber exists
  if (CORBA::is_nil (sm.subscriber ()))
    throw Manager_Exception (
        "Could not create datareader due to invalid subscriber reference");

  // use subscriber to create a datareader for the topic
  DDS::DataReader_var dr =
    sm.subscriber ()->create_datareader (topic_.in (),
                                         qos,
                                         listener_.in (),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  return dr._retn ();
}

template <typename TYPE_SUPPORT, typename TS_IMPL>
DDS::DataWriter_ptr
Topic_Manager_T <TYPE_SUPPORT, TS_IMPL>::datawriter (const Publication_Manager & pm,
                                                     const DDS::DataWriterQos & qos)
{
  // check if subscriber exists
  if (CORBA::is_nil (pm.publisher ()))
    throw Manager_Exception (
      "Could not create datawriter due to invalid publisher reference");

  // use publisher to create a datawriter for the topic
  DDS::DataWriter_var dw =
    pm.publisher ()->create_datawriter (topic_.in (),
                                        qos,
                                        DDS::DataWriterListener::_nil (),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  return dw._retn ();
}

#endif /* DDS_WRAPPER_TOPIC_MANAGER_T_CPP_ */
