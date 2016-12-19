/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H
#define OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H

#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsTypeSupportExtC.h"
#include "Definitions.h"
#include "ace/SString.h"
#include "ace/Atomic_Op.h"
#include "LocalObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  TopicDescriptionImpl(const char*            topic_name,
                       const char*            type_name,
                       TypeSupport_ptr        type_support,
                       DomainParticipantImpl* participant);

  virtual ~TopicDescriptionImpl();

  virtual char * get_type_name();

  virtual char * get_name();

  virtual DDS::DomainParticipant_ptr get_participant();

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Return the type support of the topic.
  */
  OpenDDS::DCPS::TypeSupport_ptr get_type_support();

#if !defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) || !defined(OPENDDS_NO_MULTI_TOPIC)

  bool has_reader() const {
    return reader_count_ > 0;
  }

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

#if !defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) || !defined(OPENDDS_NO_MULTI_TOPIC)
  ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> reader_count_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H  */
