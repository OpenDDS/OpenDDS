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
#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op.h>
#endif
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
* See the DDS specification, OMG formal/2015-04-10, for a description of
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

  bool has_entity_refs() const {
    return entity_refs_ > 0;
  }

  void add_entity_ref() {
    RcObject::_add_ref();
    ++entity_refs_;
  }

  void remove_entity_ref() {
    --entity_refs_;
    RcObject::_remove_ref();
  }

protected:
  /// The name of the topic.
  ACE_CString                    topic_name_;
  /// The datatype of the topic.
  ACE_CString                    type_name_;

  /// The participant that creates this topic.
  DomainParticipantImpl*         participant_;

  /// The type_support for this topic.
  OpenDDS::DCPS::TypeSupport_var type_support_;

  /// The number of entities using this topic
#ifdef ACE_HAS_CPP11
  std::atomic<uint32_t> entity_refs_;
#else
  ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> entity_refs_;
#endif
};

template <typename Topic>
class TopicDescriptionPtr
{
public:
  TopicDescriptionPtr(Topic* topic=0)
    : topic_(topic)
  {
    if (topic_)
      topic_->add_entity_ref();
  }

  ~TopicDescriptionPtr()
  {
    if (topic_)
      topic_->remove_entity_ref();
  }

  TopicDescriptionPtr(const TopicDescriptionPtr& other)
    : topic_(other.topic_)
  {
    if (topic_)
      topic_->add_entity_ref();
  }

  TopicDescriptionPtr& operator = (Topic* other)
  {
    TopicDescriptionPtr tmp(other);
    std::swap(this->topic_, tmp.topic_);
    return *this;
  }

  TopicDescriptionPtr& operator = (const TopicDescriptionPtr& other)
  {
    TopicDescriptionPtr tmp(other);
    std::swap(this->topic_, tmp.topic_);
    return *this;
  }

  Topic* operator->() const
  {
    return topic_;
  }

  Topic* get() const
  {
    return topic_;
  }

  operator bool() const
  {
    return topic_;
  }

private:
  Topic* topic_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPIC_DESCRIPTION_IMPL_H  */
