// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_TOPIC_MANAGER_H_
#define DDS_WRAPPER_TOPIC_MANAGER_H_

#include "Topic_Manager_Impl.h"
#include "DDSWrapper_export.h"

/**
 * @class ManagerException
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief class for memory management of a Topic_Manager_Impl class
 *
 * This class plays the role of an Abstraction in the Bridge pattern.
 */
class DDSWrapper_Export Topic_Manager
{
 public:
  /// default ctor
  Topic_Manager ();

  /// ctor that takes ownership of the passed in impl pointer
  Topic_Manager (Topic_Manager_Impl * impl);

  /// ctor for reference counted pointer
  Topic_Manager (Topic_Manager_Ptr impl);

  /// copy constructor
  Topic_Manager (const Topic_Manager & copy);

  /// assignment operator
  void operator= (const Topic_Manager& copy);

  /// dtor
  ~Topic_Manager ();

  /// checks for null reference
  bool null () const;

  /// getter method for the topic name
  std::string name () const;

  /// this method uses the dm participant to create a topic
  void create_topic (Domain_Manager & dm);

  /// this method uses the dm participant to create a topic
  void delete_topic (Domain_Manager & dm);

  /// this method returns a new datareader created by a subscriber
  /// memory management has to be done by the caller
  DDS::DataReader_ptr datareader (const Subscription_Manager & sm,
                                  const DDS::DataReaderQos & qos);

  /// this method returns a new datawriter created by a publisher
  /// memory management has to be done by the caller
  DDS::DataWriter_ptr datawriter (const Publication_Manager & pm,
                                  const DDS::DataWriterQos & qos);

 private:
  /// reference counted auto pointer containing the impl pointer
  Topic_Manager_Ptr manager_impl_;
};

#if defined (__ACE_INLINE__)
#include "Topic_Manager.inl"
#endif

#endif /* DDS_WRAPPER_TOPIC_MANAGER_H_ */
