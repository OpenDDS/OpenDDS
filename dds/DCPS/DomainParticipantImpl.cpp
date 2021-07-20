/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DomainParticipantImpl.h"
#include "dds/DdsDcpsGuidC.h"
#include "FeatureDisabledQosCheck.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "GuidConverter.h"
#include "PublisherImpl.h"
#include "SubscriberImpl.h"
#include "DataWriterImpl.h"
#include "Marked_Default_Qos.h"
#include "Registered_Data_Types.h"
#include "Transient_Kludge.h"
#include "DomainParticipantFactoryImpl.h"
#include "Util.h"
#include "DCPS_Utils.h"
#include "MonitorFactory.h"
#include "BitPubListenerImpl.h"
#include "ContentFilteredTopicImpl.h"
#include "MultiTopicImpl.h"
#include "transport/framework/TransportRegistry.h"
#include "transport/framework/TransportExceptions.h"

#ifdef OPENDDS_SECURITY
#include "security/framework/SecurityRegistry.h"
#include "security/framework/SecurityConfig.h"
#include "security/framework/Properties.h"
#endif

#include "RecorderImpl.h"
#include "ReplayerImpl.h"

#include "BuiltInTopicUtils.h"
#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "tao/debug.h"
#include "ace/Reactor.h"
#include "ace/OS_NS_unistd.h"

namespace Util {

  template <typename Key>
  int find(
    OpenDDS::DCPS::DomainParticipantImpl::TopicMap& c,
    const Key& key,
    OpenDDS::DCPS::DomainParticipantImpl::TopicMap::mapped_type*& value)
  {
    OpenDDS::DCPS::DomainParticipantImpl::TopicMap::iterator iter =
      c.find(key);

    if (iter == c.end()) {
      return -1;
    }

    value = &iter->second;
    return 0;
  }

  DDS::PropertySeq filter_properties(const DDS::PropertySeq& properties, const std::string& prefix)
  {
    DDS::PropertySeq result(properties.length());
    result.length(properties.length());
    unsigned int count = 0;
    for (unsigned int i = 0; i < properties.length(); ++i) {
      if (std::string(properties[i].name.in()).find(prefix) == 0) {
        result[count++] = properties[i];
      }
    }
    result.length(count);
    return result;
  }

} // namespace Util

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

//TBD - add check for enabled in most methods.
//      Currently this is not needed because auto_enable_created_entities
//      cannot be false.

// Implementation skeleton constructor
DomainParticipantImpl::DomainParticipantImpl(
  InstanceHandleGenerator& handle_generator,
  const DDS::DomainId_t& domain_id,
  const DDS::DomainParticipantQos& qos,
  DDS::DomainParticipantListener_ptr a_listener,
  const DDS::StatusMask& mask)
  : default_topic_qos_(TheServiceParticipant->initial_TopicQos()),
    default_publisher_qos_(TheServiceParticipant->initial_PublisherQos()),
    default_subscriber_qos_(TheServiceParticipant->initial_SubscriberQos()),
    qos_(qos),
#ifdef OPENDDS_SECURITY
    id_handle_(DDS::HANDLE_NIL),
    perm_handle_(DDS::HANDLE_NIL),
    part_crypto_handle_(DDS::HANDLE_NIL),
#endif
    domain_id_(domain_id),
    dp_id_(GUID_UNKNOWN),
    federated_(false),
    shutdown_condition_(shutdown_mutex_),
    shutdown_complete_(false),
    participant_handles_(handle_generator),
    pub_id_gen_(dp_id_),
    automatic_liveliness_timer_(*this),
    participant_liveliness_timer_(*this)
{
  (void) this->set_listener(a_listener, mask);
  monitor_.reset(TheServiceParticipant->monitor_factory_->create_dp_monitor(this));
  type_lookup_service_ = make_rch<XTypes::TypeLookupService>();
}

DomainParticipantImpl::~DomainParticipantImpl()
{
#ifdef OPENDDS_SECURITY
  if (security_config_ && perm_handle_ != DDS::HANDLE_NIL) {
    Security::AccessControl_var access = security_config_->get_access_control();
    DDS::Security::SecurityException se;
    if (!access->return_permissions_handle(perm_handle_, se)) {
      if (DCPS::security_debug.auth_warn) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::~DomainParticipantImpl: ")
                   ACE_TEXT("Unable to return permissions handle. SecurityException[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
    }
  }
#endif

}

DDS::Publisher_ptr
DomainParticipantImpl::create_publisher(
  const DDS::PublisherQos & qos,
  DDS::PublisherListener_ptr a_listener,
  DDS::StatusMask mask)
{
  DDS::PublisherQos pub_qos = qos;

  if (! this->validate_publisher_qos(pub_qos))
    return DDS::Publisher::_nil();

  // Although Publisher entities have GUIDs assigned (see pub_id_gen_),
  // these are not GUIDs from the RTPS spec and
  // so the handle doesn't need to correlate to the GUID.
  const DDS::InstanceHandle_t handle = assign_handle();

  PublisherImpl* pub = 0;
  ACE_NEW_RETURN(pub,
                 PublisherImpl(handle,
                               pub_id_gen_.next(),
                               pub_qos,
                               a_listener,
                               mask,
                               this),
                 DDS::Publisher::_nil());

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities)) {
    pub->enable();
  }

  DDS::Publisher_ptr pub_obj(pub);

  // this object will also act as the guard for leaking Publisher Impl
  Publisher_Pair pair(pub, pub_obj, false);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->publishers_protector_,
                   DDS::Publisher::_nil());

  if (OpenDDS::DCPS::insert(publishers_, pair) == -1) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_publisher, ")
                 ACE_TEXT("%p\n"),
                 ACE_TEXT("insert")));
    }
    return DDS::Publisher::_nil();
  }

  return DDS::Publisher::_duplicate(pub_obj);
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_publisher(
  DDS::Publisher_ptr p)
{
  // The servant's ref count should be 2 at this point,
  // one referenced by poa, one referenced by the subscriber
  // set.
  PublisherImpl* the_servant = dynamic_cast<PublisherImpl*>(p);

  if (!the_servant) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_publisher, ")
                 ACE_TEXT("Failed to obtain PublisherImpl.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!the_servant->is_clean()) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_publisher, ")
                 ACE_TEXT("The publisher is not empty.\n")));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->publishers_protector_,
                   DDS::RETCODE_ERROR);

  Publisher_Pair pair(the_servant, p, true);

  if (OpenDDS::DCPS::remove(publishers_, pair) == -1) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_publisher, ")
                 ACE_TEXT("%p\n"),
                 ACE_TEXT("remove")));
    }
    return DDS::RETCODE_ERROR;

  } else {
    return DDS::RETCODE_OK;
  }
}

DDS::Subscriber_ptr
DomainParticipantImpl::create_subscriber(
  const DDS::SubscriberQos & qos,
  DDS::SubscriberListener_ptr a_listener,
  DDS::StatusMask mask)
{
  DDS::SubscriberQos sub_qos = qos;

  if (! this->validate_subscriber_qos(sub_qos)) {
    return DDS::Subscriber::_nil();
  }

  const DDS::InstanceHandle_t handle = assign_handle();

  SubscriberImpl* sub = 0;
  ACE_NEW_RETURN(sub,
                 SubscriberImpl(handle,
                                sub_qos,
                                a_listener,
                                mask,
                                this),
                 DDS::Subscriber::_nil());

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities)) {
    sub->enable();
  }

  DDS::Subscriber_ptr sub_obj(sub);

  Subscriber_Pair pair(sub, sub_obj, false);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->subscribers_protector_,
                   DDS::Subscriber::_nil());

  if (OpenDDS::DCPS::insert(subscribers_, pair) == -1) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_subscriber, ")
                 ACE_TEXT("%p\n"),
                 ACE_TEXT("insert")));
    }
    return DDS::Subscriber::_nil();
  }

  return DDS::Subscriber::_duplicate(sub_obj);
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_subscriber(
  DDS::Subscriber_ptr s)
{
  // The servant's ref count should be 2 at this point,
  // one referenced by poa, one referenced by the subscriber
  // set.
  SubscriberImpl* the_servant = dynamic_cast<SubscriberImpl*>(s);

  if (!the_servant) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_subscriber, ")
                 ACE_TEXT("Failed to obtain SubscriberImpl.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!the_servant->is_clean()) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_subscriber, ")
                 ACE_TEXT("The subscriber is not empty.\n")));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t ret = the_servant->delete_contained_entities();
  if (ret != DDS::RETCODE_OK) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_subscriber, ")
                 ACE_TEXT("Failed to delete contained entities.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->subscribers_protector_,
                   DDS::RETCODE_ERROR);

  Subscriber_Pair pair(the_servant, s, true);

  if (OpenDDS::DCPS::remove(subscribers_, pair) == -1) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_subscriber, ")
                 ACE_TEXT("%p\n"),
                 ACE_TEXT("remove")));
    }
    return DDS::RETCODE_ERROR;

  } else {
    return DDS::RETCODE_OK;
  }
}

DDS::Subscriber_ptr
DomainParticipantImpl::get_builtin_subscriber()
{
  return DDS::Subscriber::_duplicate(bit_subscriber_.in());
}

DDS::Topic_ptr
DomainParticipantImpl::create_topic(
  const char * topic_name,
  const char * type_name,
  const DDS::TopicQos & qos,
  DDS::TopicListener_ptr a_listener,
  DDS::StatusMask mask)
{
  return create_topic_i(topic_name,
                        type_name,
                        qos,
                        a_listener,
                        mask,
                        0);
}

DDS::Topic_ptr
DomainParticipantImpl::create_typeless_topic(
  const char * topic_name,
  const char * type_name,
  bool type_has_keys,
  const DDS::TopicQos & qos,
  DDS::TopicListener_ptr a_listener,
  DDS::StatusMask mask)
{
  int topic_mask = (type_has_keys ? TOPIC_TYPE_HAS_KEYS : 0 ) | TOPIC_TYPELESS;

  return create_topic_i(topic_name,
                        type_name,
                        qos,
                        a_listener,
                        mask,
                        topic_mask);
}


DDS::Topic_ptr
DomainParticipantImpl::create_topic_i(
  const char * topic_name,
  const char * type_name,
  const DDS::TopicQos & qos,
  DDS::TopicListener_ptr a_listener,
  DDS::StatusMask mask,
  int topic_mask)
{
  DDS::TopicQos topic_qos;

  if (qos == TOPIC_QOS_DEFAULT) {
    this->get_default_topic_qos(topic_qos);

  } else {
    topic_qos = qos;
  }

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, DDS::Topic::_nil());
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, DDS::Topic::_nil());
  OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos, DDS::Topic::_nil());
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, DDS::Topic::_nil());

  if (!Qos_Helper::valid(topic_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_topic, ")
                 ACE_TEXT("invalid qos.\n")));
    }
    return DDS::Topic::_nil();
  }

  if (!Qos_Helper::consistent(topic_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_topic, ")
                 ACE_TEXT("inconsistent qos.\n")));
    }
    return DDS::Topic::_nil();
  }

  // See if there is a Topic with the same name.
  TopicMap::mapped_type* entry = 0;
  bool found = false;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->topics_protector_,
                     DDS::Topic::_nil());

#if !defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) || !defined(OPENDDS_NO_MULTI_TOPIC)
    if (topic_descrs_.count(topic_name)) {
      if (DCPS_debug_level > 3) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("DomainParticipantImpl::create_topic, ")
                   ACE_TEXT("can't create a Topic due to name \"%C\" already in use ")
                   ACE_TEXT("by a TopicDescription.\n"), topic_name));
      }
      return 0;
    }
#endif

    if (Util::find(topics_, topic_name, entry) == 0) {
      found = true;
    }
  }

  /*
   * If there is a topic with the same name, return the topic if it has the
   * same type name and QoS, else it is an error.
   */
  if (found) {
    CORBA::String_var found_type = entry->pair_.svt_->get_type_name();
    if (ACE_OS::strcmp(type_name, found_type) == 0) {
      DDS::TopicQos found_qos;
      entry->pair_.svt_->get_qos(found_qos);

      if (topic_qos == found_qos) { // match type name, qos
        {
          ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                           tao_mon,
                           this->topics_protector_,
                           DDS::Topic::_nil());
          ++entry->client_refs_;
        }
        return DDS::Topic::_duplicate(entry->pair_.obj_.in());

      } else { // Same Name and Type, Different QoS
        if (DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_topic: ")
            ACE_TEXT("topic with name \"%C\" and type %C already exists, ")
            ACE_TEXT("but the QoS doesn't match.\n"),
            topic_name, type_name));
        }

        return DDS::Topic::_nil();
      }

    } else { // Same Name, Different Type
      if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_topic: ")
          ACE_TEXT("topic with name \"%C\" already exists, but its type, %C ")
          ACE_TEXT("is not the same as %C.\n"),
          topic_name, found_type.in(), type_name));
      }

      return DDS::Topic::_nil();
    }

  } else {

    OpenDDS::DCPS::TypeSupport_var type_support;

    if (0 == topic_mask) {
       // creating a topic with compile time type
      type_support = Registered_Data_Types->lookup(this, type_name);
      if (CORBA::is_nil(type_support)) {
        if (DCPS_debug_level >= 1) {
           ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantImpl::create_topic, ")
                      ACE_TEXT("can't create a topic=%C type_name=%C ")
                      ACE_TEXT("is not registered.\n"),
                      topic_name, type_name));
        }
        return DDS::Topic::_nil();
      }
    }

    DDS::Topic_var new_topic = create_new_topic(topic_name,
                                                type_name,
                                                topic_qos,
                                                a_listener,
                                                mask,
                                                type_support);

    if (!new_topic) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("DomainParticipantImpl::create_topic, ")
                   ACE_TEXT("create_new_topic failed.\n")));
      }
      return DDS::Topic::_nil();
    }

    if ((this->enabled_ == true) && qos_.entity_factory.autoenable_created_entities) {
      if (new_topic->enable() != DDS::RETCODE_OK) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ")
                     ACE_TEXT("DomainParticipantImpl::create_topic, ")
                     ACE_TEXT("enable failed.\n")));
        }
        return DDS::Topic::_nil();
      }
    }
    return new_topic._retn();
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_topic(
  DDS::Topic_ptr a_topic)
{
  return delete_topic_i(a_topic, false);
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_topic_i(
  DDS::Topic_ptr a_topic,
  bool             remove_objref)
{
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  try {
    // The servant's ref count should be greater than 2 at this point,
    // one referenced by poa, one referenced by the topic map and
    // others referenced by the datareader/datawriter.
    TopicImpl* the_topic_servant = dynamic_cast<TopicImpl*>(a_topic);

    if (!the_topic_servant) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                   ACE_TEXT("%p\n"),
                   ACE_TEXT("failed to obtain TopicImpl.")));
      }
      return DDS::RETCODE_ERROR;
    }

    DDS::DomainParticipant_var dp = the_topic_servant->get_participant();

    DomainParticipantImpl* the_dp_servant =
      dynamic_cast<DomainParticipantImpl*>(dp.in());

    if (the_dp_servant != this) {
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DomainParticipantImpl::delete_topic_i: ")
                   ACE_TEXT("will return PRECONDITION_NOT_MET because this is not the ")
                   ACE_TEXT("participant that owns this topic\n")));
      }
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }
    if (!remove_objref && the_topic_servant->has_entity_refs()) {
      // If entity_refs is true (nonzero), then some reader or writer is using
      // this topic and the spec requires delete_topic() to fail with the error:
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DomainParticipantImpl::delete_topic_i: ")
                   ACE_TEXT("will return PRECONDITION_NOT_MET because there are still ")
                   ACE_TEXT("outstanding references to this topic\n")));
      }
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }

    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                       tao_mon,
                       this->topics_protector_,
                       DDS::RETCODE_ERROR);

      CORBA::String_var topic_name = the_topic_servant->get_name();
      TopicMap::mapped_type* entry = 0;

      TopicMapIteratorPair iters = topics_.equal_range(topic_name.in());
      TopicMapIterator iter;
      for (iter = iters.first; iter != iters.second; ++iter) {
        if (iter->second.pair_.svt_ == the_topic_servant) {
          entry = &iter->second;
          break;
        }
      }
      if (entry == 0) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, not found\n")));
        }
        return DDS::RETCODE_ERROR;
      }

      const CORBA::ULong client_refs = --entry->client_refs_;

      if (remove_objref || 0 == client_refs) {
        const GUID_t topicId = the_topic_servant->get_id();
        topics_.erase(iter);

        Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
        TopicStatus status = disco->remove_topic(
          the_dp_servant->get_domain_id(), the_dp_servant->get_id(), topicId);

        if (status != REMOVED) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                       ACE_TEXT("remove_topic failed with return value <%C>\n"),
                       topicstatus_to_string(status)));
           }
          return DDS::RETCODE_ERROR;
        }

        return DDS::RETCODE_OK;

      } else {
        if (DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DomainParticipantImpl::delete_topic_i: ")
            ACE_TEXT("Didn't remove topic from the map, remove_objref %d client_refs %d\n"),
            remove_objref, client_refs));
        }
      }
    }

  } catch (...) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                 ACE_TEXT(" Caught Unknown Exception\n")));
    }
    ret = DDS::RETCODE_ERROR;
  }

  return ret;
}

DDS::Topic_ptr
DomainParticipantImpl::find_topic(
  const char* topic_name,
  const DDS::Duration_t& timeout)
{
  const MonotonicTimePoint timeout_at(MonotonicTimePoint::now() + TimeDuration(timeout));

  bool first_time = true;
  while (first_time || MonotonicTimePoint::now() < timeout_at) {
    if (first_time) {
      first_time = false;
    }

    RepoId topic_id;
    CORBA::String_var type_name;
    DDS::TopicQos_var qos;

    Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
    TopicStatus status = disco->find_topic(domain_id_,
                                           get_id(),
                                           topic_name,
                                           type_name.out(),
                                           qos.out(),
                                           topic_id);

    const MonotonicTimePoint now = MonotonicTimePoint::now();
    if (status == FOUND) {
      OpenDDS::DCPS::TypeSupport_var type_support =
        Registered_Data_Types->lookup(this, type_name.in());
      if (CORBA::is_nil(type_support)) {
        if (DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DomainParticipantImpl::find_topic, ")
                       ACE_TEXT("can't create a Topic: type_name \"%C\" ")
                       ACE_TEXT("is not registered.\n"), type_name.in()));
        }

        return DDS::Topic::_nil();
      }

      DDS::Topic_ptr new_topic = create_new_topic(topic_name,
                                                  type_name,
                                                  qos,
                                                  DDS::TopicListener::_nil(),
                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK,
                                                  type_support);
      return new_topic;

    } else if (status == INTERNAL_ERROR) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::find_topic - ")
                   ACE_TEXT("topic not found, discovery returned INTERNAL_ERROR!\n")));
      }
      return DDS::Topic::_nil();
    } else if (now < timeout_at) {
      const TimeDuration remaining = timeout_at - now;

      if (remaining.value().sec() >= 1) {
        ACE_OS::sleep(1);

      } else {
        ACE_OS::sleep(remaining.value());
      }
    }
  }

  if (DCPS_debug_level >= 1) {
    // timed out
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::find_topic, ")
               ACE_TEXT("timed out.\n")));
  }

  return DDS::Topic::_nil();
}

DDS::TopicDescription_ptr
DomainParticipantImpl::lookup_topicdescription(const char* name)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->topics_protector_,
                   DDS::Topic::_nil());

  TopicMap::mapped_type* entry = 0;

  if (Util::find(topics_, name, entry) == -1) {
#if !defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) || !defined(OPENDDS_NO_MULTI_TOPIC)
    TopicDescriptionMap::iterator iter = topic_descrs_.find(name);
    if (iter != topic_descrs_.end()) {
      return DDS::TopicDescription::_duplicate(iter->second);
    }
#endif
    return DDS::TopicDescription::_nil();

  } else {
    return DDS::TopicDescription::_duplicate(entry->pair_.obj_.in());
  }
}

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC

DDS::ContentFilteredTopic_ptr
DomainParticipantImpl::create_contentfilteredtopic(
  const char* name,
  DDS::Topic_ptr related_topic,
  const char* filter_expression,
  const DDS::StringSeq& expression_parameters)
{
  if (CORBA::is_nil(related_topic)) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_contentfilteredtopic, ")
                 ACE_TEXT("can't create a content-filtered topic due to null related ")
                 ACE_TEXT("topic.\n")));
    }
    return 0;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_, 0);

  if (topics_.count(name)) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_contentfilteredtopic, ")
                 ACE_TEXT("can't create a content-filtered topic due to name \"%C\" ")
                 ACE_TEXT("already in use by a Topic.\n"), name));
    }
    return 0;
  }

  if (topic_descrs_.count(name)) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_contentfilteredtopic, ")
                 ACE_TEXT("can't create a content-filtered topic due to name \"%C\" ")
                 ACE_TEXT("already in use by a TopicDescription.\n"), name));
    }
    return 0;
  }

  DDS::ContentFilteredTopic_var cft;
  try {
    // Create the cft in two steps so that we only have one place to
    // check the expression parameters
    cft = new ContentFilteredTopicImpl(name, related_topic, filter_expression, this);
    if (cft->set_expression_parameters(expression_parameters) != DDS::RETCODE_OK) {
      return 0;
    }
  } catch (const std::exception& e) {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_contentfilteredtopic, ")
                 ACE_TEXT("can't create a content-filtered topic due to runtime error: ")
                 ACE_TEXT("%C.\n"), e.what()));
    }
    return 0;
  }
  DDS::TopicDescription_var td = DDS::TopicDescription::_duplicate(cft);
  topic_descrs_[name] = td;
  return cft._retn();
}

DDS::ReturnCode_t DomainParticipantImpl::delete_contentfilteredtopic(
  DDS::ContentFilteredTopic_ptr a_contentfilteredtopic)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::ContentFilteredTopic_var cft =
    DDS::ContentFilteredTopic::_duplicate(a_contentfilteredtopic);
  CORBA::String_var name = cft->get_name();
  TopicDescriptionMap::iterator iter = topic_descrs_.find(name.in());
  if (iter == topic_descrs_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_contentfilteredtopic, ")
                 ACE_TEXT("can't delete a content-filtered topic \"%C\" ")
                 ACE_TEXT("because it is not in the set.\n"), name.in ()));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  TopicDescriptionImpl* tdi = dynamic_cast<TopicDescriptionImpl*>(iter->second.in());

  if (!tdi) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_contentfilteredtopic, ")
                 ACE_TEXT("can't delete a content-filtered topic \"%C\" ")
                 ACE_TEXT("failed to obtain TopicDescriptionImpl\n"), name.in()));
    }
    return DDS::RETCODE_ERROR;
  }

  if (tdi->has_entity_refs()) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_contentfilteredtopic, ")
                 ACE_TEXT("can't delete a content-filtered topic \"%C\" ")
                 ACE_TEXT("because it is used by a datareader\n"), name.in ()));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  topic_descrs_.erase(iter);
  return DDS::RETCODE_OK;
}

#endif // OPENDDS_NO_CONTENT_FILTERED_TOPIC

#ifndef OPENDDS_NO_MULTI_TOPIC

DDS::MultiTopic_ptr DomainParticipantImpl::create_multitopic(
  const char* name, const char* type_name,
  const char* subscription_expression,
  const DDS::StringSeq& expression_parameters)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_, 0);

  if (topics_.count(name)) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_multitopic, ")
                 ACE_TEXT("can't create a multi topic due to name \"%C\" ")
                 ACE_TEXT("already in use by a Topic.\n"), name));
    }
    return 0;
  }

  if (topic_descrs_.count(name)) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_multitopic, ")
                 ACE_TEXT("can't create a multi topic due to name \"%C\" ")
                 ACE_TEXT("already in use by a TopicDescription.\n"), name));
    }
    return 0;
  }

  DDS::MultiTopic_var mt;
  try {
    mt = new MultiTopicImpl(name, type_name, subscription_expression,
      expression_parameters, this);
  } catch (const std::exception& e) {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_multitopic, ")
                 ACE_TEXT("can't create a multi topic due to runtime error: ")
                 ACE_TEXT("%C.\n"), e.what()));
    }
    return 0;
  }
  DDS::TopicDescription_var td = DDS::TopicDescription::_duplicate(mt);
  topic_descrs_[name] = td;
  return mt._retn();
}

DDS::ReturnCode_t DomainParticipantImpl::delete_multitopic(
  DDS::MultiTopic_ptr a_multitopic)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::MultiTopic_var mt = DDS::MultiTopic::_duplicate(a_multitopic);
  CORBA::String_var mt_name = mt->get_name();
  TopicDescriptionMap::iterator iter = topic_descrs_.find(mt_name.in());
  if (iter == topic_descrs_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_multitopic, ")
                 ACE_TEXT("can't delete a multitopic \"%C\" ")
                 ACE_TEXT("because it is not in the set.\n"), mt_name.in ()));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  TopicDescriptionImpl* tdi = dynamic_cast<TopicDescriptionImpl*>(iter->second.in());

  if (!tdi) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_multitopic, ")
                 ACE_TEXT("can't delete a multitopic topic \"%C\" ")
                 ACE_TEXT("failed to obtain TopicDescriptionImpl.\n"),
                 mt_name.in()));
    }
    return DDS::RETCODE_ERROR;
  }

  if (tdi->has_entity_refs()) {
    if (DCPS_debug_level > 3) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::delete_multitopic, ")
                 ACE_TEXT("can't delete a multitopic topic \"%C\" ")
                 ACE_TEXT("because it is used by a datareader.\n"), mt_name.in ()));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  topic_descrs_.erase(iter);
  return DDS::RETCODE_OK;
}

#endif // OPENDDS_NO_MULTI_TOPIC

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

RcHandle<FilterEvaluator>
DomainParticipantImpl::get_filter_eval(const char* filter)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, filter_cache_lock_,
                   RcHandle<FilterEvaluator>());

  RcHandle<FilterEvaluator>& result = filter_cache_[filter];
  if (!result) {
    try {
      result = make_rch<FilterEvaluator>(filter, false);
    } catch (const std::exception& e) {
      filter_cache_.erase(filter);
      if (DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("DomainParticipantImpl::get_filter_eval, ")
                   ACE_TEXT("can't create a writer-side content filter due to ")
                   ACE_TEXT("runtime error: %C.\n"), e.what()));
      }
    }
  }
  return result;
}

void
DomainParticipantImpl::deref_filter_eval(const char* filter)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, filter_cache_lock_);
  typedef std::map<OPENDDS_STRING, RcHandle<FilterEvaluator> > Map;
  Map::iterator iter = filter_cache_.find(filter);
  if (iter != filter_cache_.end()) {
    if (iter->second->ref_count() == 1) {
      filter_cache_.erase(iter);
    }
  }
}

#endif

DDS::ReturnCode_t
DomainParticipantImpl::delete_contained_entities()
{
  if (!get_deleted()) {
    // mark that the entity is being deleted
    set_deleted(true);

    if (!prepare_to_delete_datawriters()) {
      return DDS::RETCODE_ERROR;
    }
    if (!set_wait_pending_deadline(TheServiceParticipant->new_pending_timeout_deadline())) {
      return DDS::RETCODE_ERROR;
    }
  }

  // BIT subscriber and data readers will be deleted with the
  // rest of the entities, so need to report to discovery that
  // BIT is no longer available
  Discovery_rch disc = TheServiceParticipant->get_discovery(this->domain_id_);
  if (disc)
    disc->fini_bit(this);

  if (ACE_OS::thr_equal(TheServiceParticipant->reactor_owner(),
                        ACE_Thread::self())) {
    handle_exception(0);

  } else {
    TheServiceParticipant->reactor()->notify(this);

    shutdown_mutex_.acquire();
    while (!shutdown_complete_) {
      shutdown_condition_.wait();
    }
    shutdown_complete_ = false;
    shutdown_mutex_.release();
  }

  bit_subscriber_ = DDS::Subscriber::_nil();

  OpenDDS::DCPS::Registered_Data_Types->unregister_participant(this);

  // the participant can now start creating new contained entities
  set_deleted(false);
  return shutdown_result_;
}

CORBA::Boolean
DomainParticipantImpl::contains_entity(DDS::InstanceHandle_t a_handle)
{
  /// Check top-level containers for Topic, Subscriber,
  /// and Publisher instances.
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->topics_protector_,
                     false);

    for (TopicMap::iterator it(topics_.begin());
         it != topics_.end(); ++it) {
      if (a_handle == it->second.pair_.svt_->get_instance_handle())
        return true;
    }
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->subscribers_protector_,
                     false);

    for (SubscriberSet::iterator it(subscribers_.begin());
         it != subscribers_.end(); ++it) {
      if (a_handle == it->svt_->get_instance_handle())
        return true;
    }
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->publishers_protector_,
                     false);

    for (PublisherSet::iterator it(publishers_.begin());
         it != publishers_.end(); ++it) {
      if (a_handle == it->svt_->get_instance_handle())
        return true;
    }
  }

  /// Recurse into SubscriberImpl and PublisherImpl for
  /// DataReader and DataWriter instances respectively.
  for (SubscriberSet::iterator it(subscribers_.begin());
       it != subscribers_.end(); ++it) {
    if (it->svt_->contains_reader(a_handle))
      return true;
  }

  for (PublisherSet::iterator it(publishers_.begin());
       it != publishers_.end(); ++it) {
    if (it->svt_->contains_writer(a_handle))
      return true;
  }

  return false;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_qos(
  const DDS::DomainParticipantQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
      const bool status =
        disco->update_domain_participant_qos(domain_id_,
                                             dp_id_,
                                             qos_);

      if (!status) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) DomainParticipantImpl::set_qos, ")
                     ACE_TEXT("failed on compatibility check.\n")));
        }
        return DDS::RETCODE_ERROR;
      }
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::get_qos(
  DDS::DomainParticipantQos & qos)
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_listener(
  DDS::DomainParticipantListener_ptr a_listener,
  DDS::StatusMask mask)
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::DomainParticipantListener::_duplicate(a_listener);
  return DDS::RETCODE_OK;
}

DDS::DomainParticipantListener_ptr
DomainParticipantImpl::get_listener()
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  return DDS::DomainParticipantListener::_duplicate(listener_.in());
}

DDS::ReturnCode_t
DomainParticipantImpl::ignore_participant(
  DDS::InstanceHandle_t handle)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_participant, ")
                 ACE_TEXT("Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  RepoId ignoreId = get_repoid(handle);
  HandleMap::const_iterator location = this->ignored_participants_.find(ignoreId);

  if (location == this->ignored_participants_.end()) {
    this->ignored_participants_[ ignoreId] = handle;
  }
  else {// ignore same participant again, just return ok.
    return DDS::RETCODE_OK;
  }

  if (DCPS_debug_level >= 4) {
    GuidConverter converter(dp_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_participant: ")
               ACE_TEXT("%C ignoring handle %x.\n"),
               OPENDDS_STRING(converter).c_str(),
               handle));
  }

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  if (!disco->ignore_domain_participant(domain_id_,
                                        dp_id_,
                                        ignoreId)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_participant, ")
                 ACE_TEXT("Could not ignore domain participant.\n")));
    }
    return DDS::RETCODE_ERROR;
  }


  if (DCPS_debug_level >= 4) {
    GuidConverter converter(dp_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_participant: ")
               ACE_TEXT("%C repo call returned.\n"),
               OPENDDS_STRING(converter).c_str()));
  }

  return DDS::RETCODE_OK;
#else
  ACE_UNUSED_ARG(handle);
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::ignore_topic(
  DDS::InstanceHandle_t handle)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_topic, ")
                 ACE_TEXT(" Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  RepoId ignoreId = get_repoid(handle);
  HandleMap::const_iterator location = this->ignored_topics_.find(ignoreId);

  if (location == this->ignored_topics_.end()) {
    this->ignored_topics_[ ignoreId] = handle;
  }
  else { // ignore same topic again, just return ok.
    return DDS::RETCODE_OK;
  }

  if (DCPS_debug_level >= 4) {
    GuidConverter converter(dp_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_topic: ")
               ACE_TEXT("%C ignoring handle %x.\n"),
               OPENDDS_STRING(converter).c_str(),
               handle));
  }

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  if (!disco->ignore_topic(domain_id_,
                           dp_id_,
                           ignoreId)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_topic, ")
                 ACE_TEXT(" Could not ignore topic.\n")));
    }
  }

  return DDS::RETCODE_OK;
#else
  ACE_UNUSED_ARG(handle);
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::ignore_publication(
  DDS::InstanceHandle_t handle)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_publication, ")
                 ACE_TEXT(" Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  if (DCPS_debug_level >= 4) {
    GuidConverter converter(dp_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_publication: ")
               ACE_TEXT("%C ignoring handle %x.\n"),
               OPENDDS_STRING(converter).c_str(),
               handle));
  }

  RepoId ignoreId = get_repoid(handle);
  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  if (!disco->ignore_publication(domain_id_,
                                 dp_id_,
                                 ignoreId)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_publication, ")
                 ACE_TEXT(" could not ignore publication in discovery.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
#else
  ACE_UNUSED_ARG(handle);
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::ignore_subscription(
  DDS::InstanceHandle_t handle)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_subscription, ")
                 ACE_TEXT(" Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  if (DCPS_debug_level >= 4) {
    GuidConverter converter(dp_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_subscription: ")
               ACE_TEXT("%C ignoring handle %d.\n"),
               OPENDDS_STRING(converter).c_str(),
               handle));
  }

  RepoId ignoreId = get_repoid(handle);
  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
  if (!disco->ignore_subscription(domain_id_,
                                  dp_id_,
                                  ignoreId)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_subscription, ")
                 ACE_TEXT(" could not ignore subscription in discovery.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
#else
  ACE_UNUSED_ARG(handle);
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::DomainId_t
DomainParticipantImpl::get_domain_id()
{
  return domain_id_;
}

DDS::ReturnCode_t
DomainParticipantImpl::assert_liveliness()
{
  // This operation needs to only be used if the DomainParticipant contains
  // DataWriter entities with the LIVELINESS set to MANUAL_BY_PARTICIPANT and
  // it only affects the liveliness of those DataWriter entities. Otherwise,
  // it has no effect.
  // This will do nothing in current implementation since we only
  // support the AUTOMATIC liveliness qos for datawriter.
  // Add implementation here.

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->publishers_protector_,
                   DDS::RETCODE_ERROR);

  for (PublisherSet::iterator it(publishers_.begin());
       it != publishers_.end(); ++it) {
    it->svt_->assert_liveliness_by_participant();
  }

  last_liveliness_activity_.set_to_now();

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_publisher_qos(
  const DDS::PublisherQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_publisher_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::get_default_publisher_qos(
  DDS::PublisherQos & qos)
{
  qos = default_publisher_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_subscriber_qos(
  const DDS::SubscriberQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_subscriber_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::get_default_subscriber_qos(
  DDS::SubscriberQos & qos)
{
  qos = default_subscriber_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_topic_qos(
  const DDS::TopicQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_topic_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::get_default_topic_qos(
  DDS::TopicQos & qos)
{
  qos = default_topic_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_current_time(DDS::Time_t& current_time)
{
  current_time = SystemTimePoint::now().to_dds_time();
  return DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_participants(DDS::InstanceHandleSeq& participant_handles)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::RETCODE_ERROR);

  const CountedHandleMap::const_iterator itEnd = handles_.end();
  for (CountedHandleMap::const_iterator iter = handles_.begin(); iter != itEnd; ++iter) {
    GuidConverter converter(iter->first);

    if (converter.entityKind() == KIND_PARTICIPANT) {
      // skip itself and the ignored participant
      if (iter->first == dp_id_ || ignored_participants_.count(iter->first)) {
        continue;
      }

      push_back(participant_handles, iter->second.first);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_participant_data(
  DDS::ParticipantBuiltinTopicData & participant_data,
  DDS::InstanceHandle_t participant_handle)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::RETCODE_ERROR);

    bool found = false;
    const CountedHandleMap::const_iterator itEnd = handles_.end();
    for (CountedHandleMap::const_iterator iter = handles_.begin(); iter != itEnd; ++iter) {
      GuidConverter converter(iter->first);

      if (participant_handle == iter->second.first
          && converter.entityKind() == KIND_PARTICIPANT) {
        found = true;
        break;
      }
    }

    if (!found)
      return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::SampleInfoSeq info;
  DDS::ParticipantBuiltinTopicDataSeq data;
  DDS::DataReader_var dr =
    this->bit_subscriber_->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
  DDS::ParticipantBuiltinTopicDataDataReader_var bit_part_dr =
    DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr);
  DDS::ReturnCode_t ret = bit_part_dr->read_instance(data,
                                                     info,
                                                     1,
                                                     participant_handle,
                                                     DDS::ANY_SAMPLE_STATE,
                                                     DDS::ANY_VIEW_STATE,
                                                     DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_OK) {
    if (info[0].valid_data)
      participant_data = data[0];

    else
      return DDS::RETCODE_NO_DATA;
  }

  return ret;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_topics(DDS::InstanceHandleSeq& topic_handles)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::RETCODE_ERROR);

  const CountedHandleMap::const_iterator itEnd = handles_.end();
  for (CountedHandleMap::const_iterator iter = handles_.begin(); iter != itEnd; ++iter) {
    GuidConverter converter(iter->first);
    if (converter.isTopic()) {
      if (ignored_topics_.count(iter->first)) {
        continue;
      }

      push_back(topic_handles, iter->second.first);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_topic_data(
  DDS::TopicBuiltinTopicData & topic_data,
  DDS::InstanceHandle_t topic_handle)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::RETCODE_ERROR);

    bool found = false;
    const CountedHandleMap::const_iterator itEnd = handles_.end();
    for (CountedHandleMap::const_iterator iter = handles_.begin(); iter != itEnd; ++iter) {
      GuidConverter converter(iter->first);
      if (topic_handle == iter->second.first && converter.isTopic()) {
        found = true;
        break;
      }
    }

    if (!found)
      return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::DataReader_var dr =
    bit_subscriber_->lookup_datareader(BUILT_IN_TOPIC_TOPIC);
  DDS::TopicBuiltinTopicDataDataReader_var bit_topic_dr =
    DDS::TopicBuiltinTopicDataDataReader::_narrow(dr);

  DDS::SampleInfoSeq info;
  DDS::TopicBuiltinTopicDataSeq data;
  DDS::ReturnCode_t ret =
    bit_topic_dr->read_instance(data,
                                info,
                                1,
                                topic_handle,
                                DDS::ANY_SAMPLE_STATE,
                                DDS::ANY_VIEW_STATE,
                                DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_OK) {
    if (info[0].valid_data)
      topic_data = data[0];

    else
      return DDS::RETCODE_NO_DATA;
  }

  return ret;
}

#endif

DDS::ReturnCode_t
DomainParticipantImpl::enable()
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

#ifdef OPENDDS_SECURITY
  if (!security_config_ && TheServiceParticipant->get_security()) {
    security_config_ = TheSecurityRegistry->default_config();
    if (!security_config_) {
      security_config_ = TheSecurityRegistry->builtin_config();
      TheSecurityRegistry->default_config(security_config_);
    }
  }
#endif

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);

  if (disco.is_nil()) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                ACE_TEXT("no discovery found for domain id: %d.\n"), domain_id_));
    }
    return DDS::RETCODE_ERROR;
  }

#ifdef OPENDDS_SECURITY
  if (TheServiceParticipant->get_security() && !security_config_) {
    if (DCPS::security_debug.new_entity_error) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                 ACE_TEXT("DCPSSecurity flag is set, but unable to load security plugin configuration.\n")));
    }
    return DDS::RETCODE_ERROR;
  }
#endif

  AddDomainStatus value = {GUID_UNKNOWN, false};

#ifdef OPENDDS_SECURITY
  if (TheServiceParticipant->get_security() && security_config_->qos_implies_security(qos_)) {
    Security::Authentication_var auth = security_config_->get_authentication();

    DDS::Security::SecurityException se;
    DDS::Security::ValidationResult_t val_res =
      auth->validate_local_identity(id_handle_, dp_id_, domain_id_, qos_, disco->generate_participant_guid(), se);

    /* TODO - Handle VALIDATION_PENDING_RETRY */
    if (val_res != DDS::Security::VALIDATION_OK) {
      if (DCPS::security_debug.new_entity_error) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                   ACE_TEXT("Unable to validate local identity. SecurityException[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
    }

    Security::AccessControl_var access = security_config_->get_access_control();

    perm_handle_ = access->validate_local_permissions(auth, id_handle_, domain_id_, qos_, se);

    if (perm_handle_ == DDS::HANDLE_NIL) {
      if (DCPS::security_debug.new_entity_error) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                   ACE_TEXT("Unable to validate local permissions. SecurityException[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
    }

    const bool check_create = access->check_create_participant(perm_handle_, domain_id_, qos_, se);
    if (!check_create) {
      if (DCPS::security_debug.new_entity_error) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                   ACE_TEXT("Unable to create participant. SecurityException[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
    }

    DDS::Security::ParticipantSecurityAttributes part_sec_attr;
    const bool check_part_sec_attr = access->get_participant_sec_attributes(perm_handle_, part_sec_attr, se);

    if (!check_part_sec_attr) {
      if (DCPS::security_debug.new_entity_error) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable,")
                   ACE_TEXT("Unable to get participant security attributes. SecurityException[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return DDS::RETCODE_ERROR;
    }

    if (part_sec_attr.is_rtps_protected) { // DDS-Security v1.1 8.4.2.4 Table 27 is_rtps_protected
      if (part_sec_attr.allow_unauthenticated_participants) {
        if (DCPS::security_debug.new_entity_error) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                     ACE_TEXT("allow_unauthenticated_participants is not possible with is_rtps_protected\n")));
        }
        return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
      }

      const Security::CryptoKeyFactory_var crypto = security_config_->get_crypto_key_factory();
      part_crypto_handle_ = crypto->register_local_participant(id_handle_, perm_handle_,
        Util::filter_properties(qos_.property.value, "dds.sec.crypto."), part_sec_attr, se);
      if (part_crypto_handle_ == DDS::HANDLE_NIL) {
        if (DCPS::security_debug.new_entity_error) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                     ACE_TEXT("Unable to register local participant. SecurityException[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
        return DDS::RETCODE_ERROR;
      }

    } else {
      part_crypto_handle_ = DDS::HANDLE_NIL;
    }

    value = disco->add_domain_participant_secure(domain_id_, qos_, dp_id_, id_handle_, perm_handle_, part_crypto_handle_);

    if (value.id == GUID_UNKNOWN) {
      if (DCPS::security_debug.new_entity_error) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                   ACE_TEXT("add_domain_participant_secure returned invalid id.\n")));
      }
      return DDS::RETCODE_ERROR;
    }

  } else {
#endif

    value = disco->add_domain_participant(domain_id_, qos_);

    if (value.id == GUID_UNKNOWN) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::enable, ")
                   ACE_TEXT("add_domain_participant returned invalid id.\n")));
      }
      return DDS::RETCODE_ERROR;
    }

#ifdef OPENDDS_SECURITY
  }
#endif

  dp_id_ = value.id;
  federated_ = value.federated;

  disco->set_type_lookup_service(domain_id_, dp_id_, type_lookup_service_);

  if (monitor_) {
    monitor_->report();
  }

  if (TheServiceParticipant->monitor_) {
    TheServiceParticipant->monitor_->report();
  }

  const DDS::ReturnCode_t ret = this->set_enabled();

  if (DCPS_debug_level > 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DomainParticipantImpl::enable: ")
               ACE_TEXT("enabled participant %C in domain %d\n"),
               OPENDDS_STRING(GuidConverter(dp_id_)).c_str(), domain_id_));
  }

  if (ret == DDS::RETCODE_OK && !TheTransientKludge->is_enabled()) {
    Discovery_rch disc = TheServiceParticipant->get_discovery(this->domain_id_);
    this->bit_subscriber_ = disc->init_bit(this);
  }

  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  if (qos_.entity_factory.autoenable_created_entities) {

    for (TopicMap::iterator it = topics_.begin(); it != topics_.end(); ++it) {
      it->second.pair_.svt_->enable();
    }

    for (PublisherSet::iterator it = publishers_.begin(); it != publishers_.end(); ++it) {
      it->svt_->enable();
    }

    for (SubscriberSet::iterator it = subscribers_.begin(); it != subscribers_.end(); ++it) {
      it->svt_->enable();
    }
  }

  return DDS::RETCODE_OK;
}

RepoId
DomainParticipantImpl::get_id() const
{
  return dp_id_;
}

OPENDDS_STRING
DomainParticipantImpl::get_unique_id()
{
  return GuidConverter(dp_id_).uniqueParticipantId();
}


DDS::InstanceHandle_t
DomainParticipantImpl::get_instance_handle()
{
  return get_entity_instance_handle(dp_id_, this);
}

DDS::InstanceHandle_t DomainParticipantImpl::assign_handle(const GUID_t& id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::HANDLE_NIL);
  if (id == GUID_UNKNOWN) {
    const DDS::InstanceHandle_t ih =
      reusable_handles_.empty() ? participant_handles_.next() : reusable_handles_.pop_front();
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DomainParticipantImpl::assign_handle: "
                 "New unmapped InstanceHandle %d\n", ih));
    }
    return ih;
  }

  const CountedHandleMap::iterator location = handles_.find(id);
  if (location == handles_.end()) {
    const DDS::InstanceHandle_t handle =
      reusable_handles_.empty() ? participant_handles_.next() : reusable_handles_.pop_front();
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DomainParticipantImpl::assign_handle: "
                 "New mapped InstanceHandle %d for %C\n",
                 handle, LogGuid(id).c_str()));
    }
    handles_[id] = std::make_pair(handle, 1);
    repoIds_[handle] = id;
    return handle;
  }

  HandleWithCounter& mapped = location->second;
  ++mapped.second;
  if (DCPS_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DomainParticipantImpl::assign_handle: "
               "Incremented refcount for InstanceHandle %d to %d\n",
               mapped.first, mapped.second));
  }
  return mapped.first;
}

DDS::InstanceHandle_t DomainParticipantImpl::lookup_handle(const GUID_t& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, DDS::HANDLE_NIL);
  const CountedHandleMap::const_iterator iter = handles_.find(id);
  return iter == handles_.end() ? DDS::HANDLE_NIL : iter->second.first;
}

void DomainParticipantImpl::return_handle(DDS::InstanceHandle_t handle)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, handle_protector_);
  const RepoIdMap::iterator r_iter = repoIds_.find(handle);
  if (r_iter == repoIds_.end()) {
    reusable_handles_.add(handle);
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DomainParticipantImpl::return_handle: "
                 "Returned unmapped InstanceHandle %d\n", handle));
    }
    return;
  }

  const CountedHandleMap::iterator h_iter = handles_.find(r_iter->second);
  if (h_iter == handles_.end()) {
    return;
  }

  if (DCPS_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DomainParticipantImpl::return_handle: "
               "Returned mapped InstanceHandle %d refcount %d\n",
               handle, h_iter->second.second));
  }

  HandleWithCounter& mapped = h_iter->second;
  if (--mapped.second == 0) {
    handles_.erase(h_iter);
    repoIds_.erase(r_iter);
    reusable_handles_.add(handle);
  }
}

GUID_t DomainParticipantImpl::get_repoid(DDS::InstanceHandle_t handle) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handle_protector_, GUID_UNKNOWN);
  const RepoIdMap::const_iterator location = repoIds_.find(handle);
  return location == repoIds_.end() ? GUID_UNKNOWN : location->second;
}

DDS::Topic_ptr
DomainParticipantImpl::create_new_topic(
  const char * topic_name,
  const char * type_name,
  const DDS::TopicQos & qos,
  DDS::TopicListener_ptr a_listener,
  const DDS::StatusMask & mask,
  OpenDDS::DCPS::TypeSupport_ptr type_support)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->topics_protector_,
                   DDS::Topic::_nil());

#ifdef OPENDDS_SECURITY
  if (perm_handle_ && !topicIsBIT(topic_name, type_name)) {
    Security::AccessControl_var access = security_config_->get_access_control();

    DDS::Security::SecurityException se;

    DDS::Security::TopicSecurityAttributes sec_attr;
    if (!access->get_topic_sec_attributes(perm_handle_, topic_name, sec_attr, se)) {
      if (DCPS::security_debug.new_entity_warn) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("DomainParticipantImpl::create_new_topic, ")
                   ACE_TEXT("Unable to get security attributes for topic '%C'. SecurityException[%d.%d]: %C\n"),
                   topic_name, se.code, se.minor_code, se.message.in()));
        }
      return DDS::Topic::_nil();
    }

    if ((sec_attr.is_write_protected || sec_attr.is_read_protected) &&
        !access->check_create_topic(perm_handle_, domain_id_, topic_name, qos, se)) {
      if (DCPS::security_debug.new_entity_warn) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("DomainParticipantImpl::create_new_topic, ")
                   ACE_TEXT("Permissions check failed to create new topic '%C'. SecurityException[%d.%d]: %C\n"),
                   topic_name, se.code, se.minor_code, se.message.in()));
      }
      return DDS::Topic::_nil();
    }
  }
#endif

  TopicImpl* topic_servant = 0;

  ACE_NEW_RETURN(topic_servant,
                 TopicImpl(topic_name,
                           type_name,
                           type_support,
                           qos,
                           a_listener,
                           mask,
                           this),
                 DDS::Topic::_nil());

  if ((enabled_ == true)
      && (qos_.entity_factory.autoenable_created_entities)) {
    const DDS::ReturnCode_t ret = topic_servant->enable();

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: ")
          ACE_TEXT("DomainParticipantImpl::create_new_topic, ")
          ACE_TEXT("enable failed.\n")));
      return DDS::Topic::_nil();
    }
  }

  DDS::Topic_ptr obj(topic_servant);

  // this object will also act as a guard against leaking the new TopicImpl
  RefCounted_Topic refCounted_topic(Topic_Pair(topic_servant, obj, false));
  topics_.insert(std::make_pair(topic_name, refCounted_topic));

  if (this->monitor_) {
    this->monitor_->report();
  }

  // the topics_ map has one reference and we duplicate to give
  // the caller another reference.
  return DDS::Topic::_duplicate(refCounted_topic.pair_.obj_.in());
}

bool
DomainParticipantImpl::is_clean() const
{
  bool sub_is_clean = subscribers_.empty();
  bool topics_is_clean = true;

  // check that the only remaining topics are built-in topics
  for (TopicMap::const_iterator it = topics_.begin(); it != topics_.end(); ++it) {
    if (!topicIsBIT(it->second.pair_.svt_->topic_name(), it->second.pair_.svt_->type_name())) {
      topics_is_clean = false;
    }
  }

  if (!TheTransientKludge->is_enabled()) {
    // There are built-in topics and built-in topic subscribers left.
    sub_is_clean = !sub_is_clean ? subscribers_.size() == 1 : true;
  }
  return (publishers_.empty()
          && sub_is_clean
          && topics_is_clean);
}

DDS::DomainParticipantListener_ptr
DomainParticipantImpl::listener_for(DDS::StatusKind kind)
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  if (CORBA::is_nil(listener_.in()) || (listener_mask_ & kind) == 0) {
    return DDS::DomainParticipantListener::_nil ();
  } else {
    return DDS::DomainParticipantListener::_duplicate(listener_.in());
  }
}

void
DomainParticipantImpl::get_topic_ids(TopicIdVec& topics)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->topics_protector_);

  topics.reserve(topics_.size());
  for (TopicMap::iterator it(topics_.begin());
       it != topics_.end(); ++it) {
    topics.push_back(it->second.pair_.svt_->get_id());
  }
}

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE

OwnershipManager*
DomainParticipantImpl::ownership_manager()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  DDS::DataReader_var dr =
    bit_subscriber_->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  DDS::PublicationBuiltinTopicDataDataReader_var bit_pub_dr =
    DDS::PublicationBuiltinTopicDataDataReader::_narrow(dr);

  if (!CORBA::is_nil(bit_pub_dr.in())) {
    DDS::DataReaderListener_var listener = bit_pub_dr->get_listener();
    if (CORBA::is_nil(listener.in())) {
      DDS::DataReaderListener_var bit_pub_listener =
        new BitPubListenerImpl(this);
      bit_pub_dr->set_listener(bit_pub_listener, DDS::DATA_AVAILABLE_STATUS);
      // Must call on_data_available when attaching a listener late - samples may be waiting
      bit_pub_listener->on_data_available(bit_pub_dr.in());
    }
  }

#endif
  return &this->owner_man_;
}

void
DomainParticipantImpl::update_ownership_strength (const PublicationId& pub_id,
                                                  const CORBA::Long& ownership_strength)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            tao_mon,
            this->subscribers_protector_);

  if (this->get_deleted ())
    return;

  for (SubscriberSet::iterator it(this->subscribers_.begin());
      it != this->subscribers_.end(); ++it) {
    it->svt_->update_ownership_strength(pub_id, ownership_strength);
  }
}

#endif // OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE

DomainParticipantImpl::RepoIdSequence::RepoIdSequence(const RepoId& base) :
  base_(base),
  serial_(0),
  builder_(base_)
{
}

RepoId
DomainParticipantImpl::RepoIdSequence::next()
{
  builder_.entityKey(++serial_);
  return builder_;
}


////////////////////////////////////////////////////////////////


bool
DomainParticipantImpl::validate_publisher_qos(DDS::PublisherQos & pub_qos)
{
  if (pub_qos == PUBLISHER_QOS_DEFAULT) {
    this->get_default_publisher_qos(pub_qos);
  }

  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(pub_qos, false);

  if (!Qos_Helper::valid(pub_qos) || !Qos_Helper::consistent(pub_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::validate_publisher_qos, ")
                 ACE_TEXT("invalid qos.\n")));
    }
    return false;
  }

  return true;
}

bool
DomainParticipantImpl::validate_subscriber_qos(DDS::SubscriberQos & subscriber_qos)
{
  if (subscriber_qos == SUBSCRIBER_QOS_DEFAULT) {
    this->get_default_subscriber_qos(subscriber_qos);
  }

  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(subscriber_qos, false);

  if (!Qos_Helper::valid(subscriber_qos) || !Qos_Helper::consistent(subscriber_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::validate_subscriber_qos, ")
                 ACE_TEXT("invalid qos.\n")));
    }
    return false;
  }


  return true;
}

Recorder_ptr
DomainParticipantImpl::create_recorder(DDS::Topic_ptr a_topic,
                                       const DDS::SubscriberQos& subscriber_qos,
                                       const DDS::DataReaderQos& datareader_qos,
                                       const RecorderListener_rch& a_listener,
                                       DDS::StatusMask mask)
{
  if (CORBA::is_nil(a_topic)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("SubscriberImpl::create_datareader, ")
                 ACE_TEXT("topic desc is nil.\n")));
    }
    return 0;
  }

  DDS::SubscriberQos sub_qos = subscriber_qos;
  DDS::DataReaderQos dr_qos;

  if (! this->validate_subscriber_qos(sub_qos) ||
      ! SubscriberImpl::validate_datareader_qos(datareader_qos,
                                                TheServiceParticipant->initial_DataReaderQos(),
                                                a_topic,
                                                dr_qos, false) ) {
    return 0;
  }

  RecorderImpl* recorder(new RecorderImpl);
  Recorder_var result(recorder);

  recorder->init(dynamic_cast<TopicDescriptionImpl*>(a_topic),
    dr_qos, a_listener,
    mask, this, subscriber_qos);

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities)) {
    recorder->enable();
  }

  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(recorders_protector_);
  recorders_.insert(result);

  return result._retn();
}

Replayer_ptr
DomainParticipantImpl::create_replayer(DDS::Topic_ptr a_topic,
                                       const DDS::PublisherQos& publisher_qos,
                                       const DDS::DataWriterQos& datawriter_qos,
                                       const ReplayerListener_rch& a_listener,
                                       DDS::StatusMask mask)
{
  if (CORBA::is_nil(a_topic)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("SubscriberImpl::create_datareader, ")
                 ACE_TEXT("topic desc is nil.\n")));
    }
    return 0;
  }

  DDS::PublisherQos pub_qos = publisher_qos;
  DDS::DataWriterQos dw_qos;

  if (! this->validate_publisher_qos(pub_qos) ||
      ! PublisherImpl::validate_datawriter_qos(datawriter_qos,
                                               TheServiceParticipant->initial_DataWriterQos(),
                                               a_topic,
                                               dw_qos)) {
    return 0;
  }

  TopicImpl* topic_servant = dynamic_cast<TopicImpl*>(a_topic);

  ReplayerImpl* replayer(new ReplayerImpl);
  Replayer_var result(replayer);

  replayer->init(a_topic, topic_servant, dw_qos, a_listener, mask, this, pub_qos);

  if ((this->enabled_ == true) && (qos_.entity_factory.autoenable_created_entities)) {
    const DDS::ReturnCode_t ret = replayer->enable();

    if (ret != DDS::RETCODE_OK) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("DomainParticipantImpl::create_replayer, ")
                   ACE_TEXT("enable failed.\n")));
      }
      return 0;
    }
  }

  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(replayers_protector_);
  replayers_.insert(result);
  return result._retn();
}

void
DomainParticipantImpl::delete_recorder(Recorder_ptr recorder)
{
  const Recorder_var recvar(Recorder::_duplicate(recorder));
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(recorders_protector_);
  recorders_.erase(recvar);
}

void
DomainParticipantImpl::delete_replayer(Replayer_ptr replayer)
{
  const Replayer_var repvar(Replayer::_duplicate(replayer));
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(replayers_protector_);
  replayers_.erase(repvar);
}

void
DomainParticipantImpl::add_adjust_liveliness_timers(DataWriterImpl* writer)
{
  automatic_liveliness_timer_.add_adjust(writer);
  participant_liveliness_timer_.add_adjust(writer);
}

void
DomainParticipantImpl::remove_adjust_liveliness_timers()
{
  automatic_liveliness_timer_.remove_adjust();
  participant_liveliness_timer_.remove_adjust();
}

DomainParticipantImpl::LivelinessTimer::LivelinessTimer(DomainParticipantImpl& impl,
                                                        DDS::LivelinessQosPolicyKind kind)
  : impl_(impl)
  , kind_(kind)
  , interval_(TimeDuration::max_value)
  , recalculate_interval_(false)
  , scheduled_(false)
{ }

DomainParticipantImpl::LivelinessTimer::~LivelinessTimer()
{
  if (scheduled_ && TheServiceParticipant->timer()) {
    TheServiceParticipant->timer()->cancel_timer(this);
  }
}

void
DomainParticipantImpl::LivelinessTimer::add_adjust(OpenDDS::DCPS::DataWriterImpl* writer)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  const MonotonicTimePoint now = MonotonicTimePoint::now();

  // Calculate the time remaining to liveliness check.
  const TimeDuration remaining = interval_ - (now - last_liveliness_check_);

  // Adopt a smaller interval.
  interval_ = std::min(interval_, writer->liveliness_check_interval(kind_));

  // Reschedule or schedule a timer if necessary.
  if (scheduled_ && interval_ < remaining) {
    TheServiceParticipant->timer()->cancel_timer(this);
    TheServiceParticipant->timer()->schedule_timer(this, 0, interval_.value());
  } else if (!scheduled_) {
    TheServiceParticipant->timer()->schedule_timer(this, 0, interval_.value());
    scheduled_ = true;
    last_liveliness_check_ = now;
  }
}

void
DomainParticipantImpl::LivelinessTimer::remove_adjust()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, this->lock_);

  recalculate_interval_ = true;
}

int
DomainParticipantImpl::LivelinessTimer::handle_timeout(
  const ACE_Time_Value& tv,
  const void* /* arg */)
{
  const MonotonicTimePoint now(tv);

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->lock_, 0);

  scheduled_ = false;

  if (recalculate_interval_) {
    interval_ = impl_.liveliness_check_interval(kind_);
    recalculate_interval_ = false;
  }

  if (!interval_.is_max()) {
    dispatch(now);
    last_liveliness_check_ = now;
    TheServiceParticipant->timer()->schedule_timer(this, 0, interval_.value());
    scheduled_ = true;
  }

  return 0;
}

DomainParticipantImpl::AutomaticLivelinessTimer::AutomaticLivelinessTimer(DomainParticipantImpl& impl)
  : LivelinessTimer (impl, DDS::AUTOMATIC_LIVELINESS_QOS)
{ }

void
DomainParticipantImpl::AutomaticLivelinessTimer::dispatch(const MonotonicTimePoint& /* tv */)
{
  impl_.signal_liveliness (kind_);
}

DomainParticipantImpl::ParticipantLivelinessTimer::ParticipantLivelinessTimer(DomainParticipantImpl& impl)
  : LivelinessTimer (impl, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)
{ }

void
DomainParticipantImpl::ParticipantLivelinessTimer::dispatch(const MonotonicTimePoint& tv)
{
  if (impl_.participant_liveliness_activity_after (tv - interval())) {
    impl_.signal_liveliness (kind_);
  }
}

TimeDuration
DomainParticipantImpl::liveliness_check_interval(DDS::LivelinessQosPolicyKind kind)
{
  TimeDuration tv(TimeDuration::max_value);

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    tao_mon,
                    this->publishers_protector_,
                    tv);

  for (PublisherSet::iterator it(publishers_.begin());
       it != publishers_.end(); ++it) {
    tv = std::min (tv, it->svt_->liveliness_check_interval(kind));
  }

  return tv;
}

bool
DomainParticipantImpl::participant_liveliness_activity_after(const MonotonicTimePoint& tv)
{
  if (last_liveliness_activity_ > tv) {
    return true;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, tao_mon, this->publishers_protector_, !tv.is_zero());

  for (PublisherSet::iterator it(publishers_.begin());
       it != publishers_.end(); ++it) {
    if (it->svt_->participant_liveliness_activity_after(tv)) {
      return true;
    }
  }

  return false;
}

void
DomainParticipantImpl::signal_liveliness (DDS::LivelinessQosPolicyKind kind)
{
  TheServiceParticipant->get_discovery(domain_id_)->signal_liveliness (domain_id_, get_id(), kind);
}

#ifdef OPENDDS_SECURITY
void
DomainParticipantImpl::set_security_config(const Security::SecurityConfig_rch& cfg)
{
  security_config_ = cfg;
}
#endif

int
DomainParticipantImpl::handle_exception(ACE_HANDLE /*fd*/)
{
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  // delete publishers
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->publishers_protector_,
                     DDS::RETCODE_ERROR);

    PublisherSet::iterator pubIter = publishers_.begin();
    DDS::Publisher_ptr pubPtr;
    size_t pubsize = publishers_.size();

    while (pubsize > 0) {
      pubPtr = (*pubIter).obj_.in();
      ++pubIter;

      DDS::ReturnCode_t result = pubPtr->delete_contained_entities();
      if (result != DDS::RETCODE_OK) {
        ret = result;
      }

      result = delete_publisher(pubPtr);

      if (result != DDS::RETCODE_OK) {
        ret = result;
      }

      --pubsize;
    }

  }

  // delete subscribers
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->subscribers_protector_,
                     DDS::RETCODE_ERROR);

    SubscriberSet::iterator subIter = subscribers_.begin();
    DDS::Subscriber_ptr subPtr;
    size_t subsize = subscribers_.size();

    while (subsize > 0) {
      subPtr = (*subIter).obj_.in();
      ++subIter;

      DDS::ReturnCode_t result = subPtr->delete_contained_entities();

      if (result != DDS::RETCODE_OK) {
        ret = result;
      }

      result = delete_subscriber(subPtr);

      if (result != DDS::RETCODE_OK) {
        ret = result;
      }

      --subsize;
    }
  }

  // delete topics
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->topics_protector_,
                     DDS::RETCODE_ERROR);

    TopicMap::iterator topicIter = topics_.begin();
    DDS::Topic_ptr topicPtr;
    size_t topicsize = topics_.size();

    while (topicsize > 0) {
      topicPtr = topicIter->second.pair_.obj_.in();
      ++topicIter;

      // Delete the topic the reference count.
      const DDS::ReturnCode_t result = this->delete_topic_i(topicPtr, true);

      if (result != DDS::RETCODE_OK) {
        ret = result;
      }
      --topicsize;
    }
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->recorders_protector_,
                     DDS::RETCODE_ERROR);

    RecorderSet::iterator it = recorders_.begin();
    for (; it != recorders_.end(); ++it ){
      RecorderImpl* impl = dynamic_cast<RecorderImpl* >(it->in());
      DDS::ReturnCode_t result = DDS::RETCODE_ERROR;
      if (impl) result = impl->cleanup();
      if (result != DDS::RETCODE_OK) ret = result;
    }
    recorders_.clear();
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->replayers_protector_,
                     DDS::RETCODE_ERROR);

    ReplayerSet::iterator it = replayers_.begin();
    for (; it != replayers_.end(); ++it ){
      ReplayerImpl* impl = static_cast<ReplayerImpl* >(it->in());
      DDS::ReturnCode_t result = DDS::RETCODE_ERROR;
      if (impl) result = impl->cleanup();
      if (result != DDS::RETCODE_OK) ret = result;

    }

    replayers_.clear();
  }

  shutdown_mutex_.acquire();
  shutdown_result_ = ret;
  shutdown_complete_ = true;
  shutdown_condition_.notify_all();
  shutdown_mutex_.release();

  return 0;
}

bool DomainParticipantImpl::prepare_to_delete_datawriters()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, publishers_protector_, false);
  bool result = true;
  const PublisherSet::iterator end = publishers_.end();
  for (PublisherSet::iterator i = publishers_.begin(); i != end; ++i) {
    result &= i->svt_->prepare_to_delete_datawriters();
  }
  return result;
}

bool DomainParticipantImpl::set_wait_pending_deadline(const MonotonicTimePoint& deadline)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, publishers_protector_, false);
  bool result = true;
  const PublisherSet::iterator end = publishers_.end();
  for (PublisherSet::iterator i = publishers_.begin(); i != end; ++i) {
    result &= i->svt_->set_wait_pending_deadline(deadline);
  }
  return result;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
