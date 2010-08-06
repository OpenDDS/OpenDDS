/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H
#define OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H

#include "dds/DdsDcpsTopicS.h"
#include "dds/DdsDcpsTypeSupportExtS.h"
#include "ace/SString.h"
#include "ace/Atomic_Op.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;

/**
* @class TopicDescriptionImpl
*
* @brief Implements the DDS::TopicDescription interface.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*
*/
class OpenDDS_Dcps_Export TopicDescriptionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::TopicDescription> {
public:

  //Constructor
  TopicDescriptionImpl(const char*            topic_name,
                       const char*            type_name,
                       TypeSupport_ptr        type_support,
                       DomainParticipantImpl* participant);

  //Destructor
  virtual ~TopicDescriptionImpl();

  virtual char * get_type_name()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual char * get_name()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DomainParticipant_ptr get_participant()
  ACE_THROW_SPEC((CORBA::SystemException));

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Return the type support of the topic.
  */
  OpenDDS::DCPS::TypeSupport_ptr get_type_support();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  /// only used for ContentFilteredTopic and MultiTopic
  bool has_reader() const {
    return reader_count_ > 0;
  }

  /// only used for ContentFilteredTopic and MultiTopic
  void update_reader_count(bool increment) {
    if (increment) ++reader_count_;
    else --reader_count_;
  }

#endif

protected:
  /// The name of the topic.
  ACE_CString                    topic_name_;
  /// The datatype of the topic.
  ACE_CString                    type_name_;

  /// The participant that creates this topic.
  DomainParticipantImpl*         participant_;

  /// The type_support for this topic.
  OpenDDS::DCPS::TypeSupport_ptr type_support_;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> reader_count_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H  */
