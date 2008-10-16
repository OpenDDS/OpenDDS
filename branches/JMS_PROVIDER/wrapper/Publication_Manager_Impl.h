// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager_Impl.h
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _PUBLICATION_MANAGER_IMPL_H_
#define _PUBLICATION_MANAGER_IMPL_H_

#include <dds/DdsDcpsPublicationC.h>

#include "Reference_Counter_T.h"

/// forward declaration
class Topic_Manager;

/**
 * @class Publication_Manager_Impl
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief interface for all publication manager implementations
 */
class Publication_Manager_Impl 
{
  /// this friend declaration is needed for reference counting purposes
  friend class Reference_Counter_T <Publication_Manager_Impl>;

 public:
  // default ctor
  Publication_Manager_Impl ();

  /// destructor
  virtual ~Publication_Manager_Impl ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  virtual DDS::DataWriter_ptr access_topic (const Topic_Manager & topic) = 0;

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic) = 0;

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the 
  /// Publication_Manager_Impl itself
  virtual DDS::Publisher_ptr publisher () const = 0;

 protected:
  /// reference count variable
  unsigned long use_;
};

#endif /* _PUBLICATION_MANAGER_IMPL_H_ */
