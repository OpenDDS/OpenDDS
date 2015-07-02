// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_PUBLICATION_MANAGER_H_
#define DDS_WRAPPER_PUBLICATION_MANAGER_H_

#include "DDSWrapper_export.h"
#include "Publication_Manager_Impl.h"

/**
 * @class Publication_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief class for memory management of a Publication_ManagerImpl class
 *
 * This class plays the role of an Abstraction in the Bridge pattern.
 */
class DDSWrapper_Export Publication_Manager
{
 public:
  /// default ctor
  Publication_Manager ();

  /// ctor that takes ownership of the passed in impl pointer
  Publication_Manager (const Publication_Manager_Ptr & impl);

  /// copy constructor
  Publication_Manager (const Publication_Manager & copy);

  /// assignment operator
  void operator= (const Publication_Manager & copy);

  /// checks for null reference
  bool null () const;

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  DDS::DataWriter_ptr access_topic (const Topic_Manager & topic,
                                    const DDS::DataWriterQos & qos
                                      = DATAWRITER_QOS_DEFAULT);

  /// unregisters and deletes the topic from the domain
  void remove_topic (const Topic_Manager & topic);

  /// creates and returns qos for data writers with the default values
  DDS::DataWriterQos get_default_datawriter_qos ();

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the
  /// Publication_Manager itself
  DDS::Publisher_ptr publisher () const;

 private:
  /// reference counted auto pointer containing the impl pointer
  Publication_Manager_Ptr manager_impl_;
};

#if defined (__ACE_INLINE__)
#include "Publication_Manager.inl"
#endif

#endif /* DDS_WRAPPER_PUBLICATION_MANAGER_H_ */
