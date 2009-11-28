/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H
#define TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H

#include "dds/DdsDcpsTopicS.h"
#include "dds/DdsDcpsTypeSupportExtS.h"
#include "ace/SString.h"

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

protected:
  /// The name of the topic.
  ACE_CString                    topic_name_;
  /// The datatype of the topic.
  ACE_CString                    type_name_;

  /// The participant that creates this topic.
  DomainParticipantImpl*         participant_;

  /// The type_support for this topic.
  OpenDDS::DCPS::TypeSupport_ptr type_support_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* TAO_DDS_DCPS_TOPIC_DESCRIPTION_IMPL_H  */
