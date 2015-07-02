// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager_T.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_TOPIC_MANAGER_T_H_
#define DDS_WRAPPER_TOPIC_MANAGER_T_H_

#include "Topic_Manager_Impl.h"

/**
 * @class Topic_Manager_T
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief template based implementation class of the the Topic_Manager interface
 */
template <typename TYPE_SUPPORT, typename TS_IMPL>
class Topic_Manager_T : public Topic_Manager_Impl
{
 public:
  /// ctor
  /// @param name of the topic to be created
  /// @param type name of the registered type support for this topic
  /// @param listener is used to create a datareader with the datareader
  ///                 method.
  Topic_Manager_T (const std::string& name,
                   DDS::DataReaderListener_ptr listener =
                     DDS::DataReaderListener::_nil (),
                   bool create_new_topic = true);

  /// dtor
  virtual ~Topic_Manager_T ();

  /// getter method for the topic name
  virtual std::string name () const;

  /// this method uses the dm participant to create a topic
  virtual void create_topic (Domain_Manager & dm);

  /// this method uses the dm participant to create a topic
  virtual void delete_topic (Domain_Manager & dm);

  /// this method returns a new datareader created by a subscriber
  /// memory management has to be done by the caller
  virtual DDS::DataReader_ptr datareader (const Subscription_Manager & sm,
                                          const DDS::DataReaderQos & qos);

  /// this method returns a new datawriter created by a publisher
  /// memory management has to be done by the caller
  virtual DDS::DataWriter_ptr datawriter (const Publication_Manager & pm,
                                          const DDS::DataWriterQos & qos);

 private:
  /// copy ctor is private to prevent copies
  Topic_Manager_T (const Topic_Manager_T & orig);

  /// assignment operator is private to prevent copies
  void operator= (const Topic_Manager_T & rhs);

 private:
  /// name of the topic to be registered
  std::string name_;

  /// The managed topic
  DDS::Topic_var topic_;

  /// reference to the type support object
  typename TYPE_SUPPORT::_var_type type_;

  /// ReaderListener reference
  DDS::DataReaderListener_var listener_;

  /// determines if a new topic should be created on registration in the domain
  bool create_new_topic_;
};

#include "Topic_Manager_T.cpp"

#endif /* DDS_WRAPPER_TOPIC_MANAGER_T_H_ */
