// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager.h
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _PUBLICATION_MANAGER_H_
#define _PUBLICATION_MANAGER_H_

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
  Publication_Manager (Publication_Manager_Impl * impl);

  /// copy constructor
  Publication_Manager (const Publication_Manager & copy);

  /// assignment operator
  void operator= (const Publication_Manager& copy);

  /// destructor
  ~Publication_Manager ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  DDS::DataWriter_ptr access_topic (const Topic_Manager & topic);

  /// unregisters and deletes the topic from the domain
  void remove_topic (const Topic_Manager & topic);

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the 
  /// Publication_Manager itself
  DDS::Publisher_ptr publisher () const;

 private:
  Reference_Counter_T <Publication_Manager_Impl> manager_impl_;
};

#if defined (__ACE_INLINE__)
#include "Publication_Manager.inl"
#endif

#endif /* _PUBLICATION_MANAGER_H_ */
