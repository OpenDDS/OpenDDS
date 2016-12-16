// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager_Impl.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_PUBLICATION_MANAGER_IMPL_H_
#define DDS_WRAPPER_PUBLICATION_MANAGER_IMPL_H_

#include <ace/Refcounted_Auto_Ptr.h>
#include <ace/Null_Mutex.h>
#include "wrapper_publication.h"

/// forward declarations
class Topic_Manager;
class Publication_Manager_Impl;

/// this defines a reference counted pointer for a publication manager
/// implementation
typedef class ACE_Refcounted_Auto_Ptr <Publication_Manager_Impl,
                                       ACE_Null_Mutex> Publication_Manager_Ptr;

/**
 * @class Publication_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all publication manager implementations
 */
class Publication_Manager_Impl
{
 public:
  virtual ~Publication_Manager_Impl ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  /// @param ref is needed since access topic needs to copy the
  ///            publication_manager internally and therefore needs the correct
  ///            reference count
  virtual DDS::DataWriter_ptr access_topic (
    const Topic_Manager & topic,
    const DDS::DataWriterQos & qos,
    const Publication_Manager_Ptr & ref) = 0;

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic) = 0;

  /// creates and returns qos for data writers with the default values
  virtual DDS::DataWriterQos get_default_datawriter_qos () = 0;

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the
  /// Publication_Manager_Impl itself
  virtual DDS::Publisher_ptr publisher () const = 0;
};

#endif /* DDS_WRAPPER_PUBLICATION_MANAGER_IMPL_H_ */
