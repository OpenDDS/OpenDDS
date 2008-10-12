// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager_Impl.h
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _TOPIC_MANAGER_IMPL_H_
#define _TOPIC_MANAGER_IMPL_H_

#include <string>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsPublicationC.h>
#include "DDSWrapper_export.h"
#include "Reference_Counter_T.h"

/// forward declarations
class Domain_Manager;
class Subscription_Manager;
class Publication_Manager;

/**
 * @class ManagerException
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief abstract interface for topic creation and usage
 */
class DDSWrapper_Export Topic_Manager_Impl
{
  /// this friend declaration is needed for reference counting purposes
  friend class Reference_Counter_T <Topic_Manager_Impl>;

 public:
  /// ctor
  Topic_Manager_Impl ();

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
  virtual DDS::DataReader_ptr datareader (const Subscription_Manager & sm) = 0;

  /// this method returns a new datawriter created by a publisher
  /// memory management has to be done by the caller
  virtual DDS::DataWriter_ptr datawriter (const Publication_Manager & pm) = 0;

 protected:
  /// reference counter variable
  unsigned long use_;
};

#endif /* _TOPIC_MANAGER_IMPL_H_ */
