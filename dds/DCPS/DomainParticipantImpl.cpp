/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DomainParticipantImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "PublisherImpl.h"
#include "SubscriberImpl.h"
#include "Marked_Default_Qos.h"
#include "Registered_Data_Types.h"
#include "Transient_Kludge.h"
#include "FailoverListener.h"
#include "DomainParticipantFactoryImpl.h"
#include "Util.h"
#include "MonitorFactory.h"
#include "dds/DdsDcpsGuidC.h"
#include "BitPubListenerImpl.h"
#include "ContentFilteredTopicImpl.h"
#include "MultiTopicImpl.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include <sstream>

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "tao/debug.h"

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

} // namespace Util

namespace OpenDDS {
namespace DCPS {

//TBD - add check for enabled in most methods.
//      Currently this is not needed because auto_enable_created_entities
//      cannot be false.

// Implementation skeleton constructor
DomainParticipantImpl::DomainParticipantImpl(DomainParticipantFactoryImpl *       factory,
                                             const DDS::DomainId_t&             domain_id,
                                             const RepoId&                        dp_id,
                                             const DDS::DomainParticipantQos &  qos,
                                             DDS::DomainParticipantListener_ptr a_listener,
                                             const DDS::StatusMask &            mask,
                                             bool                                 federated)
  : factory_(factory),
    default_topic_qos_(TheServiceParticipant->initial_TopicQos()),
    default_publisher_qos_(TheServiceParticipant->initial_PublisherQos()),
    default_subscriber_qos_(TheServiceParticipant->initial_SubscriberQos()),
    qos_(qos),
    domain_id_(domain_id),
    dp_id_(dp_id),
    federated_(federated),
    failoverListener_(0),
    monitor_(0),
    pub_id_generator_ (
      0,
      OpenDDS::DCPS::RepoIdConverter(this->dp_id_).participantId(),
      OpenDDS::DCPS::KIND_PUBLISHER)
{
  (void) this->set_listener(a_listener, mask);
  monitor_ = TheServiceParticipant->monitor_factory_->create_dp_monitor(this);
}

// Implementation skeleton destructor
DomainParticipantImpl::~DomainParticipantImpl()
{
  delete this->failoverListener_;
}

DDS::Publisher_ptr
DomainParticipantImpl::create_publisher(
  const DDS::PublisherQos & qos,
  DDS::PublisherListener_ptr a_listener,
  DDS::StatusMask mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_UNUSED_ARG(mask);

  DDS::PublisherQos pub_qos;

  if (qos == PUBLISHER_QOS_DEFAULT) {
    this->get_default_publisher_qos(pub_qos);

  } else {
    pub_qos = qos;
  }

  if (!Qos_Helper::valid(pub_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_publisher, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::Publisher::_nil();
  }

  if (!Qos_Helper::consistent(pub_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_publisher, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::Publisher::_nil();
  }

  PublisherImpl* pub = 0;
  ACE_NEW_RETURN(pub,
                 PublisherImpl(participant_handles_.next(),
                               pub_id_generator_.next (),
                               pub_qos,
                               a_listener,
                               mask,
                               this),
                 DDS::Publisher::_nil());

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities == 1)) {
    pub->enable();
  }

  DDS::Publisher_ptr pub_obj(pub);

  // this object will also act as the guard for leaking Publisher Impl
  Publisher_Pair pair(pub, pub_obj, NO_DUP);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->publishers_protector_,
                   DDS::Publisher::_nil());

  if (OpenDDS::DCPS::insert(publishers_, pair) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_publisher, ")
               ACE_TEXT("%p\n"),
               ACE_TEXT("insert")));
    return DDS::Publisher::_nil();
  }

  return DDS::Publisher::_duplicate(pub_obj);
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_publisher(
  DDS::Publisher_ptr p)
ACE_THROW_SPEC((CORBA::SystemException))
{
  // The servant's ref count should be 2 at this point,
  // one referenced by poa, one referenced by the subscriber
  // set.
  PublisherImpl* the_servant = dynamic_cast<PublisherImpl*>(p);

  if (the_servant->is_clean() == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::delete_publisher, ")
               ACE_TEXT("The publisher is not empty.\n")));
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->publishers_protector_,
                   DDS::RETCODE_ERROR);

  Publisher_Pair pair(the_servant, p, DUP);

  if (OpenDDS::DCPS::remove(publishers_, pair) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_publisher, ")
               ACE_TEXT("%p\n"),
               ACE_TEXT("remove")));
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::SubscriberQos sub_qos;

  if (qos == SUBSCRIBER_QOS_DEFAULT) {
    this->get_default_subscriber_qos(sub_qos);

  } else {
    sub_qos = qos;
  }

  if (!Qos_Helper::valid(sub_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_subscriber, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::Subscriber::_nil();
  }

  if (!Qos_Helper::consistent(sub_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_subscriber, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::Subscriber::_nil();
  }

  SubscriberImpl* sub = 0 ;
  ACE_NEW_RETURN(sub,
                 SubscriberImpl(participant_handles_.next(),
                                sub_qos,
                                a_listener,
                                mask,
                                this),
                 DDS::Subscriber::_nil());

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities == 1)) {
    sub->enable();
  }

  DDS::Subscriber_ptr sub_obj(sub);

  Subscriber_Pair pair(sub, sub_obj, NO_DUP);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->subscribers_protector_,
                   DDS::Subscriber::_nil());

  if (OpenDDS::DCPS::insert(subscribers_, pair) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_subscriber, ")
               ACE_TEXT("%p\n"),
               ACE_TEXT("insert")));
    return DDS::Subscriber::_nil();
  }

  return DDS::Subscriber::_duplicate(sub_obj);
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_subscriber(
  DDS::Subscriber_ptr s)
ACE_THROW_SPEC((CORBA::SystemException))
{
  // The servant's ref count should be 2 at this point,
  // one referenced by poa, one referenced by the subscriber
  // set.
  SubscriberImpl* the_servant = dynamic_cast<SubscriberImpl*>(s);

  if (the_servant->is_clean() == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::delete_subscriber, ")
               ACE_TEXT("The subscriber is not empty.\n")));
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t ret
  = the_servant->delete_contained_entities();

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::delete_subscriber, ")
               ACE_TEXT("Failed to delete contained entities.\n")));
    return DDS::RETCODE_ERROR;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->subscribers_protector_,
                   DDS::RETCODE_ERROR);

  Subscriber_Pair pair(the_servant, s, DUP);

  if (OpenDDS::DCPS::remove(subscribers_, pair) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_subscriber, ")
               ACE_TEXT("%p\n"),
               ACE_TEXT("remove")));
    return DDS::RETCODE_ERROR;

  } else {
    return DDS::RETCODE_OK;
  }
}

DDS::Subscriber_ptr
DomainParticipantImpl::get_builtin_subscriber()
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::TopicQos topic_qos;

  if (qos == TOPIC_QOS_DEFAULT) {
    this->get_default_topic_qos(topic_qos);

  } else {
    topic_qos = qos;
  }

  if (!Qos_Helper::valid(topic_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_topic, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::Topic::_nil();
  }

  if (!Qos_Helper::consistent(topic_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantImpl::create_topic, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::Topic::_nil();
  }

  TopicMap::mapped_type* entry = 0;
  bool found = false;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->topics_protector_,
                     DDS::Topic::_nil());

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
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

  if (found) {
    CORBA::String_var found_type
    = entry->pair_.svt_->get_type_name();

    if (ACE_OS::strcmp(type_name, found_type) == 0) {
      DDS::TopicQos found_qos;
      entry->pair_.svt_->get_qos(found_qos);

      if (topic_qos == found_qos) { // match type name, qos
        {
          ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                           tao_mon,
                           this->topics_protector_,
                           DDS::Topic::_nil());
          entry->client_refs_ ++;
        }
        return DDS::Topic::_duplicate(entry->pair_.obj_.in());

      } else {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DomainParticipantImpl::create_topic, ")
                     ACE_TEXT("qos not match: topic_name=%C type_name=%C\n"),
                     topic_name, type_name));
        }

        return DDS::Topic::_nil();
      }

    } else { // no match
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DomainParticipantImpl::create_topic, ")
                   ACE_TEXT(" not match: topic_name=%C type_name=%C\n"),
                   topic_name, type_name));
      }

      return DDS::Topic::_nil();
    }

  } else {
    RepoId topic_id;

    try {
      DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
      TopicStatus status = repo->assert_topic(topic_id,
                                              domain_id_,
                                              dp_id_,
                                              topic_name,
                                              type_name,
                                              topic_qos);

      if (status == CREATED || status == FOUND) {
        DDS::Topic_ptr new_topic = create_topic_i(topic_id,
                                                  topic_name,
                                                  type_name,
                                                  topic_qos,
                                                  a_listener,
                                                  mask);
        if (this->monitor_) {
          this->monitor_->report();
        }
        return new_topic;

      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_topic, ")
                   ACE_TEXT("assert_topic failed.\n")));
        return DDS::Topic::_nil();
      }

    } catch (const CORBA::SystemException& sysex) {
      sysex._tao_print_exception(
        "ERROR: System Exception"
        " in DomainParticipantImpl::create_topic");
      return DDS::Topic::_nil();

    } catch (const CORBA::UserException& userex) {
      userex._tao_print_exception(
        "ERROR: User Exception"
        "in DomainParticipantImpl::create_topic");
      return DDS::Topic::_nil();
    }
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::delete_topic(
  DDS::Topic_ptr a_topic)
ACE_THROW_SPEC((CORBA::SystemException))
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

    CORBA::String_var topic_name = the_topic_servant->get_name();

    DDS::DomainParticipant_var dp = the_topic_servant->get_participant();

    DomainParticipantImpl* the_dp_servant =
      dynamic_cast<DomainParticipantImpl*>(dp.in());

    if (the_dp_servant != this) {
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }

    {

      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                       tao_mon,
                       this->topics_protector_,
                       DDS::RETCODE_ERROR);

      TopicMap::mapped_type* entry = 0;

      if (Util::find(topics_, topic_name.in(), entry) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                          ACE_TEXT("%p\n"),
                          ACE_TEXT("find")),
                         DDS::RETCODE_ERROR);
      }

      entry->client_refs_ --;

      if (remove_objref == true ||
          0 == entry->client_refs_) {
        //TBD - mark the TopicImpl as deleted and make it
        //      reject calls to the TopicImpl.
        DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
        TopicStatus status
        = repo->remove_topic(the_dp_servant->get_domain_id(),
                             the_dp_servant->get_id(),
                             the_topic_servant->get_id());

        if (status != REMOVED) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                            ACE_TEXT("remove_topic failed\n")),
                           DDS::RETCODE_ERROR);
        }

        // note: this will destroy the TopicImpl if there are no
        // client object reference to it.
        if (topics_.erase(topic_name.in()) == 0) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
                            ACE_TEXT("%p \n"),
                            ACE_TEXT("unbind")),
                           DDS::RETCODE_ERROR);

        } else
          return DDS::RETCODE_OK;

      }
    }

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in DomainParticipantImpl::delete_topic_i");
    ret = DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::delete_topic_i");
    ret = DDS::RETCODE_ERROR;

  } catch (...) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::delete_topic_i, ")
               ACE_TEXT(" Caught Unknown Exception \n")));
    ret = DDS::RETCODE_ERROR;
  }

  return ret;
}

//Note: caller should NOT assign to Topic_var (without _duplicate'ing)
//      because it will steal the framework's reference.
DDS::Topic_ptr
DomainParticipantImpl::find_topic(
  const char * topic_name,
  const DDS::Duration_t & timeout)
ACE_THROW_SPEC((CORBA::SystemException))
{
  try {
    ACE_Time_Value timeout_tv
    = ACE_OS::gettimeofday() + ACE_Time_Value(timeout.sec, timeout.nanosec/1000);

    int first_time = 1;

    while (first_time || ACE_OS::gettimeofday() < timeout_tv) {
      if (first_time) {
        first_time = 0;
      }

      TopicMap::mapped_type* entry = 0;
      {
        ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                         tao_mon,
                         this->topics_protector_,
                         DDS::Topic::_nil());

        if (Util::find(topics_, topic_name, entry) == 0) {
          entry->client_refs_ ++;
          return DDS::Topic::_duplicate(entry->pair_.obj_.in());
        }
      }

      RepoId topic_id;
      CORBA::String_var type_name;
      DDS::TopicQos_var qos;

      DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
      TopicStatus status = repo->find_topic(domain_id_,
                                            topic_name,
                                            type_name.out(),
                                            qos.out(),
                                            topic_id);

      if (status == FOUND) {
        DDS::Topic_ptr new_topic = create_topic_i(topic_id,
                                                    topic_name,
                                                    type_name,
                                                    qos,
                                                    DDS::TopicListener::_nil(),
                                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        return new_topic;

      } else {
        ACE_Time_Value now = ACE_OS::gettimeofday();

        if (now < timeout_tv) {
          ACE_Time_Value remaining = timeout_tv - now;

          if (remaining.sec() >= 1) {
            ACE_OS::sleep(1);

          } else {
            ACE_OS::sleep(remaining);
          }
        }
      }
    }

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in DomainParticipantImpl::find_topic");
    return DDS::Topic::_nil();

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::find_topic");
    return DDS::Topic::_nil();
  }

  if (DCPS_debug_level >= 1) {
    // timed out
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantImpl::find_topic, ")
               ACE_TEXT("timed out. \n")));
  }

  return DDS::Topic::_nil();
}

DDS::TopicDescription_ptr
DomainParticipantImpl::lookup_topicdescription(const char* name)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->topics_protector_,
                   DDS::Topic::_nil());

  TopicMap::mapped_type* entry = 0;

  if (Util::find(topics_, name, entry) == -1) {
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
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


#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

DDS::ContentFilteredTopic_ptr
DomainParticipantImpl::create_contentfilteredtopic(
  const char* name,
  DDS::Topic_ptr related_topic,
  const char* filter_expression,
  const DDS::StringSeq& expression_parameters)
ACE_THROW_SPEC((CORBA::SystemException))
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
    cft = new ContentFilteredTopicImpl(name,
      related_topic, filter_expression, expression_parameters, this);
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::ContentFilteredTopic_var cft =
    DDS::ContentFilteredTopic::_duplicate(a_contentfilteredtopic);
  CORBA::String_var name = cft->get_name();
  TopicDescriptionMap::iterator iter = topic_descrs_.find(name.in());
  if (iter == topic_descrs_.end()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  if (dynamic_cast<TopicDescriptionImpl*>(iter->second.in())->has_reader()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  topic_descrs_.erase(iter);
  return DDS::RETCODE_OK;
}

DDS::MultiTopic_ptr DomainParticipantImpl::create_multitopic(
  const char* name, const char* type_name,
  const char* subscription_expression,
  const DDS::StringSeq& expression_parameters)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, topics_protector_,
                   DDS::RETCODE_OUT_OF_RESOURCES);
  DDS::MultiTopic_var mt = DDS::MultiTopic::_duplicate(a_multitopic);
  TopicDescriptionMap::iterator iter = topic_descrs_.find(mt->get_name());
  if (iter == topic_descrs_.end()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  if (dynamic_cast<TopicDescriptionImpl*>(iter->second.in())->has_reader()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  topic_descrs_.erase(iter);
  return DDS::RETCODE_OK;
}

RcHandle<FilterEvaluator>
DomainParticipantImpl::get_filter_eval(const char* filter)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, filter_cache_lock_,
                   RcHandle<FilterEvaluator>());
  typedef std::map<std::string, RcHandle<FilterEvaluator> > Map;
  Map::iterator iter = filter_cache_.find(filter);
  if (iter == filter_cache_.end()) {
    return filter_cache_[filter] = new FilterEvaluator(filter, false);
  }
  return iter->second;
}

void
DomainParticipantImpl::deref_filter_eval(const char* filter)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, filter_cache_lock_);
  typedef std::map<std::string, RcHandle<FilterEvaluator> > Map;
  Map::iterator iter = filter_cache_.find(filter);
  if (iter != filter_cache_.end()) {
    if (iter->second->ref_count() == 1) {
      filter_cache_.erase(iter);
    }
  }
}

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

DDS::ReturnCode_t
DomainParticipantImpl::delete_contained_entities()
ACE_THROW_SPEC((CORBA::SystemException))
{
  // mark that the entity is being deleted
  set_deleted(true);

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

      DDS::ReturnCode_t result
      = pubPtr->delete_contained_entities();

      if (result != DDS::RETCODE_OK) {
        return result;
      }

      result = delete_publisher(pubPtr);

      if (result != DDS::RETCODE_OK) {
        return result;
      }

      pubsize--;
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
        return result;
      }

      result = delete_subscriber(subPtr);

      if (result != DDS::RETCODE_OK) {
        return result;
      }

      subsize--;
    }
  }

  DDS::ReturnCode_t ret = DDS::RETCODE_OK;
  // delete topics
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->topics_protector_,
                     DDS::RETCODE_ERROR);

    while (1) {
      if (topics_.begin() == topics_.end()) {
        break;
      }

      // Delete the topic the reference count.
      DDS::ReturnCode_t result = this->delete_topic_i(
                                     topics_.begin()->second.pair_.obj_.in(), true);

      if (result != DDS::RETCODE_OK) {
        ret = result;
      }
    }
  }

#if !defined (DDS_HAS_MINIMUM_BIT)
  bit_part_topic_ = DDS::Topic::_nil();
  bit_topic_topic_ = DDS::Topic::_nil();
  bit_pub_topic_ = DDS::Topic::_nil();
  bit_sub_topic_ = DDS::Topic::_nil();

  bit_part_dr_ = DDS::ParticipantBuiltinTopicDataDataReader::_nil();
  bit_topic_dr_ = DDS::TopicBuiltinTopicDataDataReader::_nil();
  bit_pub_dr_ = DDS::PublicationBuiltinTopicDataDataReader::_nil();
  bit_sub_dr_ = DDS::SubscriptionBuiltinTopicDataDataReader::_nil();
#endif

  bit_subscriber_ = DDS::Subscriber::_nil();

  OpenDDS::DCPS::Registered_Data_Types->unregister_participant(this);
  participant_objref_ = DDS::DomainParticipant::_nil();

  // the participant can now start creating new contained entities
  set_deleted(false);

  return ret;
}

CORBA::Boolean
DomainParticipantImpl::contains_entity(DDS::InstanceHandle_t a_handle)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      try {
        DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
        CORBA::Boolean status
        = repo->update_domain_participant_qos(domain_id_,
                                              dp_id_,
                                              qos_);

        if (status == 0) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipantImpl::set_qos, ")
                            ACE_TEXT("failed on compatiblity check. \n")),
                           DDS::RETCODE_ERROR);
        }

      } catch (const CORBA::SystemException& sysex) {
        sysex._tao_print_exception(
          "ERROR: System Exception"
          " in DomainParticipantImpl::set_qos");
        return DDS::RETCODE_ERROR;

      } catch (const CORBA::UserException& userex) {
        userex._tao_print_exception(
          "ERROR:  Exception"
          " in DomainParticipantImpl::set_qos");
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_listener(
  DDS::DomainParticipantListener_ptr a_listener,
  DDS::StatusMask mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::DomainParticipantListener::_duplicate(a_listener);
  fast_listener_ = listener_.in();
  return DDS::RETCODE_OK;
}

DDS::DomainParticipantListener_ptr
DomainParticipantImpl::get_listener()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return DDS::DomainParticipantListener::_duplicate(listener_.in());
}

DDS::ReturnCode_t
DomainParticipantImpl::ignore_participant(
  DDS::InstanceHandle_t handle)
ACE_THROW_SPEC((CORBA::SystemException))
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_participant, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  RepoId ignoreId = RepoIdBuilder::create();

  BIT_Helper_1 < DDS::ParticipantBuiltinTopicDataDataReader,
  DDS::ParticipantBuiltinTopicDataDataReader_var,
  DDS::ParticipantBuiltinTopicDataSeq > hh;
  DDS::ReturnCode_t ret
  = hh.instance_handle_to_repo_key(this, BUILT_IN_PARTICIPANT_TOPIC, handle, ignoreId);

  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  HandleMap::const_iterator location = this->ignored_participants_.find(ignoreId);

  if (location == this->ignored_participants_.end()) {
    this->ignored_participants_[ ignoreId] = handle;
  }
  else {// ignore same participant again, just return ok.
    return DDS::RETCODE_OK;
  }

  try {
    if (DCPS_debug_level >= 4) {
      RepoIdConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_participant: ")
                 ACE_TEXT("%C ignoring handle %x.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
    repo->ignore_domain_participant(domain_id_,
                                    dp_id_,
                                    ignoreId);

    if (DCPS_debug_level >= 4) {
      RepoIdConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_participant: ")
                 ACE_TEXT("%C repo call returned.\n"),
                 std::string(converter).c_str()));
    }

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in DomainParticipantImpl::ignore_participant");
    return DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::ignore_participant");
    return DDS::RETCODE_ERROR;
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
ACE_THROW_SPEC((CORBA::SystemException))
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_topic, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  RepoId ignoreId = RepoIdBuilder::create();

  BIT_Helper_1 < DDS::TopicBuiltinTopicDataDataReader,
  DDS::TopicBuiltinTopicDataDataReader_var,
  DDS::TopicBuiltinTopicDataSeq > hh;
  DDS::ReturnCode_t ret =
    hh.instance_handle_to_repo_key(this, BUILT_IN_TOPIC_TOPIC, handle, ignoreId);

  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  HandleMap::const_iterator location = this->ignored_topics_.find(ignoreId);

  if (location == this->ignored_topics_.end()) {
    this->ignored_topics_[ ignoreId] = handle;
  }
  else { // ignore same topic again, just return ok.
    return DDS::RETCODE_OK;
  }

  try {
    if (DCPS_debug_level >= 4) {
      RepoIdConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_topic: ")
                 ACE_TEXT("%C ignoring handle %x.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
    repo->ignore_topic(domain_id_,
                       dp_id_,
                       ignoreId);

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "System Exception"
      " in DomainParticipantImpl::ignore_topic");
    return DDS::RETCODE_OK;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::ignore_topic");
    return DDS::RETCODE_OK;
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
ACE_THROW_SPEC((CORBA::SystemException))
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_publication, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  RepoId ignoreId = RepoIdBuilder::create();

  BIT_Helper_1 < DDS::PublicationBuiltinTopicDataDataReader,
  DDS::PublicationBuiltinTopicDataDataReader_var,
  DDS::PublicationBuiltinTopicDataSeq > hh;
  DDS::ReturnCode_t ret =
    hh.instance_handle_to_repo_key(this, BUILT_IN_PUBLICATION_TOPIC, handle, ignoreId);

  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  try {
    if (DCPS_debug_level >= 4) {
      RepoIdConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_publication: ")
                 ACE_TEXT("%C ignoring handle %x.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
    repo->ignore_publication(domain_id_,
                             dp_id_,
                             ignoreId);

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in DomainParticipantImpl::ignore_publication");
    return DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::ignore_publication");
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
ACE_THROW_SPEC((CORBA::SystemException))
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (enabled_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::ignore_subscription, ")
                      ACE_TEXT(" Entity is not enabled. \n")),
                     DDS::RETCODE_NOT_ENABLED);
  }

  RepoId ignoreId = RepoIdBuilder::create();

  BIT_Helper_1 < DDS::SubscriptionBuiltinTopicDataDataReader,
  DDS::SubscriptionBuiltinTopicDataDataReader_var,
  DDS::SubscriptionBuiltinTopicDataSeq > hh;
  DDS::ReturnCode_t ret =
    hh.instance_handle_to_repo_key(this, BUILT_IN_SUBSCRIPTION_TOPIC, handle, ignoreId);

  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  try {
    if (DCPS_debug_level >= 4) {
      RepoIdConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_subscription: ")
                 ACE_TEXT("%C ignoring handle %d.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
    repo->ignore_subscription(domain_id_,
                              dp_id_,
                              ignoreId);

  } catch (const CORBA::SystemException& sysex) {
    sysex._tao_print_exception(
      "ERROR: System Exception"
      " in DomainParticipantImpl::ignore_subscription");
    return DDS::RETCODE_ERROR;

  } catch (const CORBA::UserException& userex) {
    userex._tao_print_exception(
      "ERROR: User Exception"
      " in DomainParticipantImpl::ignore_subscription");
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  return domain_id_;
}

DDS::ReturnCode_t
DomainParticipantImpl::assert_liveliness()
ACE_THROW_SPEC((CORBA::SystemException))
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

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_publisher_qos(
  const DDS::PublisherQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = default_publisher_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_subscriber_qos(
  const DDS::SubscriberQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = default_subscriber_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::set_default_topic_qos(
  const DDS::TopicQos & qos)
ACE_THROW_SPEC((CORBA::SystemException))
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  qos = default_topic_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_current_time(
  DDS::Time_t & current_time)
ACE_THROW_SPEC((CORBA::SystemException))
{
  current_time
  = OpenDDS::DCPS::time_value_to_time(
      ACE_OS::gettimeofday());
  return DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_participants(
  DDS::InstanceHandleSeq & participant_handles)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->handle_protector_,
                   DDS::RETCODE_ERROR);

  HandleMap::const_iterator itEnd = this->handles_.end();

  for (HandleMap::const_iterator iter = this->handles_.begin();
       iter != itEnd; ++iter) {
    GuidConverter converter(iter->first);

    if (converter.entityKind() == KIND_PARTICIPANT)
    {
      // skip itself and the ignored participant
      if (iter->first == this->dp_id_
      || (this->ignored_participants_.find(iter->first)
        != this->ignored_participants_.end ())) {
        continue;
      }

      push_back(participant_handles, iter->second);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_participant_data(
  DDS::ParticipantBuiltinTopicData & participant_data,
  DDS::InstanceHandle_t participant_handle)
ACE_THROW_SPEC((CORBA::SystemException))
{
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->handle_protector_,
                     DDS::RETCODE_ERROR);

    bool found = false;
    HandleMap::const_iterator itEnd = this->handles_.end();

    for (HandleMap::const_iterator iter = this->handles_.begin();
         iter != itEnd; ++iter) {
      GuidConverter converter(iter->first);

      if (participant_handle == iter->second
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
  DDS::ReturnCode_t ret
  = this->bit_part_dr_->read_instance(data,
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
DomainParticipantImpl::get_discovered_topics(
  DDS::InstanceHandleSeq & topic_handles)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->handle_protector_,
                   DDS::RETCODE_ERROR);

  HandleMap::const_iterator itEnd = this->handles_.end();

  for (HandleMap::const_iterator iter = this->handles_.begin();
       iter != itEnd; ++iter) {
    GuidConverter converter(iter->first);

    if (converter.entityKind() == KIND_TOPIC) {

      // skip the ignored topic
      if (this->ignored_topics_.find(iter->first)
          != this->ignored_topics_.end ()) {
        continue;
      }

      push_back(topic_handles, iter->second);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::get_discovered_topic_data(
  DDS::TopicBuiltinTopicData & topic_data,
  DDS::InstanceHandle_t topic_handle)
ACE_THROW_SPEC((CORBA::SystemException))
{
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->handle_protector_,
                     DDS::RETCODE_ERROR);

    bool found = false;
    HandleMap::const_iterator itEnd = this->handles_.end();

    for (HandleMap::const_iterator iter = this->handles_.begin();
         iter != itEnd; ++iter) {
      GuidConverter converter(iter->first);

      if (topic_handle == iter->second
          && converter.entityKind() == KIND_TOPIC) {
        found = true;
        break;
      }
    }

    if (!found)
      return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::SampleInfoSeq info;
  DDS::TopicBuiltinTopicDataSeq data;
  DDS::ReturnCode_t ret
  = this->bit_topic_dr_->read_instance(data,
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
ACE_THROW_SPEC((CORBA::SystemException))
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  DDS::DomainParticipantFactoryQos qos;

  if (this->factory_->get_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t)DomainParticipantImpl::enable failed to")
               ACE_TEXT(" get factory qos\n")));
    return DDS::RETCODE_ERROR;
  }

  if (qos.entity_factory.autoenable_created_entities == 0) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t ret = this->set_enabled();

  if (monitor_) {
    monitor_->report();
  }
  if (TheServiceParticipant->monitor_) {
    TheServiceParticipant->monitor_->report();
  }

  if (ret == DDS::RETCODE_OK && !TheTransientKludge->is_enabled()) {
#if !defined (DDS_HAS_MINIMUM_BIT)

    if (TheServiceParticipant->get_BIT()) {
      return init_bit();

    } else {
      return DDS::RETCODE_OK;
    }

#else
    return DDS::RETCODE_OK;
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  } else {
    return ret;
  }
}

RepoId
DomainParticipantImpl::get_id()
{
  return dp_id_;
}

DDS::InstanceHandle_t
DomainParticipantImpl::get_instance_handle()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return this->get_handle(this->dp_id_);
}

CORBA::Long
DomainParticipantImpl::get_federation_id()
ACE_THROW_SPEC((CORBA::SystemException))
{
  RepoIdConverter converter(dp_id_);
  return converter.federationId();
}

CORBA::Long
DomainParticipantImpl::get_participant_id()
ACE_THROW_SPEC((CORBA::SystemException))
{
  RepoIdConverter converter(dp_id_);
  return converter.participantId();
}

DDS::InstanceHandle_t
DomainParticipantImpl::get_handle(const RepoId& id)
{
  if (id == GUID_UNKNOWN) {
    return this->participant_handles_.next();
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->handle_protector_,
                   HANDLE_UNKNOWN);

  HandleMap::const_iterator location = this->handles_.find(id);

  if (location == this->handles_.end()) {
    this->handles_[ id] = this->participant_handles_.next();
  }

  return this->handles_[ id];
}

DDS::Topic_ptr
DomainParticipantImpl::create_topic_i(
  const RepoId topic_id,
  const char * topic_name,
  const char * type_name,
  const DDS::TopicQos & qos,
  DDS::TopicListener_ptr a_listener,
  const DDS::StatusMask & mask)
ACE_THROW_SPEC((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->topics_protector_,
                   DDS::Topic::_nil());

  TopicMap::mapped_type* entry = 0;

  if (Util::find(topics_, topic_name, entry) == 0) {
    entry->client_refs_ ++;
    return DDS::Topic::_duplicate(entry->pair_.obj_.in());
  }

  OpenDDS::DCPS::TypeSupport_ptr type_support =
    OpenDDS::DCPS::Registered_Data_Types->lookup(this->participant_objref_.in(),type_name);

  if (0 == type_support) {
    return DDS::Topic::_nil();
  }

  TopicImpl* topic_servant;

  ACE_NEW_RETURN(topic_servant,
                 TopicImpl(topic_id,
                           topic_name,
                           type_name,
                           type_support,
                           qos,
                           a_listener,
                           mask,
                           this),
                 DDS::Topic::_nil());

  if ((enabled_ == true) && (qos_.entity_factory.autoenable_created_entities == 1)) {
    topic_servant->enable();
  }

  DDS::Topic_ptr obj(topic_servant);

  // this object will also act as a guard against leaking the new TopicImpl
  RefCounted_Topic refCounted_topic(Topic_Pair(topic_servant, obj, NO_DUP));

  if (OpenDDS::DCPS::bind(topics_, topic_name, refCounted_topic) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::create_topic, ")
               ACE_TEXT("%p \n"),
               ACE_TEXT("bind")));
    return DDS::Topic::_nil();
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  // the topics_ map has one reference and we duplicate to give
  // the caller another reference.
  return DDS::Topic::_duplicate(refCounted_topic.pair_.obj_.in());
}

int
DomainParticipantImpl::is_clean() const
{
  int sub_is_clean = subscribers_.empty();
  int topics_is_clean = topics_.size() == 0;

  if (!TheTransientKludge->is_enabled()) {
    // There are four topics and builtin topic subscribers
    // left.

    sub_is_clean = sub_is_clean == 0 ? subscribers_.size() == 1 : 1;
    topics_is_clean = topics_is_clean == 0 ? topics_.size() == 4 : 1;
  }

  return (publishers_.empty()
          && sub_is_clean == 1
          && topics_is_clean == 1);
}

void
DomainParticipantImpl::set_object_reference(const DDS::DomainParticipant_ptr& dp)
{
  if (!CORBA::is_nil(participant_objref_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::set_object_reference, ")
               ACE_TEXT("This participant is already activated. \n")));
    return;
  }

  participant_objref_ = DDS::DomainParticipant::_duplicate(dp);
}

DDS::DomainParticipantListener*
DomainParticipantImpl::listener_for(DDS::StatusKind kind)
{
  if (fast_listener_ == 0 || (listener_mask_ & kind) == 0) {
    return 0;

  } else {
    return fast_listener_;
  }
}

DDS::ReturnCode_t
DomainParticipantImpl::init_bit()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  DDS::ReturnCode_t ret;

  if (((ret = init_bit_subscriber()) == DDS::RETCODE_OK)
      && ((ret = attach_bit_transport()) == DDS::RETCODE_OK)
      && ((ret = init_bit_topics()) == DDS::RETCODE_OK)
      && ((ret = init_bit_datareaders()) == DDS::RETCODE_OK)) {
    return DDS::RETCODE_OK;

  } else {
    return ret;
  }

#else
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::init_bit_topics()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  try {
    DDS::TopicQos topic_qos;
    this->get_default_topic_qos(topic_qos);

    OpenDDS::DCPS::TypeSupport_ptr type_support =
      OpenDDS::DCPS::Registered_Data_Types->lookup(this->participant_objref_.in(), BUILT_IN_PARTICIPANT_TOPIC_TYPE);

    if (0 == type_support) {
      // Participant topic
      DDS::ParticipantBuiltinTopicDataTypeSupport_var participantTypeSupport_servant(
        new DDS::ParticipantBuiltinTopicDataTypeSupportImpl());
      DDS::ReturnCode_t ret
      = participantTypeSupport_servant->register_type(participant_objref_.in(),
                                                      BUILT_IN_PARTICIPANT_TOPIC_TYPE);

      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                          ACE_TEXT("register BUILT_IN_PARTICIPANT_TOPIC_TYPE returned %d.\n"),
                          ret),
                         ret);
      }
    }

    bit_part_topic_ = this->create_topic(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                         OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                                         topic_qos,
                                         DDS::TopicListener::_nil(),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bit_part_topic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                        ACE_TEXT("Nil %C Topic \n"),
                        OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC),
                       DDS::RETCODE_ERROR);
    }

    // Topic topic
    type_support =
      OpenDDS::DCPS::Registered_Data_Types->lookup(this->participant_objref_.in(), BUILT_IN_TOPIC_TOPIC_TYPE);

    if (0 == type_support) {
      DDS::TopicBuiltinTopicDataTypeSupport_var topicTypeSupport_servant(
        new DDS::TopicBuiltinTopicDataTypeSupportImpl());

      DDS::ReturnCode_t ret
      = topicTypeSupport_servant->register_type(participant_objref_.in(),
                                                BUILT_IN_TOPIC_TOPIC_TYPE);

      if (ret != DDS::RETCODE_OK) {

        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                          ACE_TEXT("register BUILT_IN_TOPIC_TOPIC_TYPE returned %d.\n"),
                          ret),
                         ret);
      }
    }

    bit_topic_topic_ = this->create_topic(OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC,
                                          OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE,
                                          topic_qos,
                                          DDS::TopicListener::_nil(),
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bit_topic_topic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                        ACE_TEXT("Nil %C Topic \n"),
                        OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC),
                       DDS::RETCODE_ERROR);
    }

    // Subscription topic
    type_support =
      OpenDDS::DCPS::Registered_Data_Types->lookup(this->participant_objref_.in(), BUILT_IN_SUBSCRIPTION_TOPIC_TYPE);

    if (0 == type_support) {
      DDS::SubscriptionBuiltinTopicDataTypeSupport_var subscriptionTypeSupport_servant(
        new DDS::SubscriptionBuiltinTopicDataTypeSupportImpl());

      DDS::ReturnCode_t ret
      = subscriptionTypeSupport_servant->register_type(participant_objref_.in(),
                                                       BUILT_IN_SUBSCRIPTION_TOPIC_TYPE);

      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                          ACE_TEXT("register BUILT_IN_SUBSCRIPTION_TOPIC_TYPE returned %d.\n"),
                          ret),
                         ret);
      }
    }

    bit_sub_topic_ =
      this->create_topic(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC,
                         OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                         topic_qos,
                         DDS::TopicListener::_nil(),
                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bit_sub_topic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                        ACE_TEXT("Nil %C Topic \n"),
                        OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC),
                       DDS::RETCODE_ERROR);
    }

    // Publication topic
    type_support =
      OpenDDS::DCPS::Registered_Data_Types->lookup(this->participant_objref_.in(), BUILT_IN_PUBLICATION_TOPIC_TYPE);

    if (0 == type_support) {
      DDS::PublicationBuiltinTopicDataTypeSupport_var publicationTypeSupport_servant(
        new DDS::PublicationBuiltinTopicDataTypeSupportImpl());

      DDS::ReturnCode_t ret
      = publicationTypeSupport_servant->register_type(participant_objref_.in(),
                                                      BUILT_IN_PUBLICATION_TOPIC_TYPE);

      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("DomainParticipantImpl::init_bit_topics, ")
                          ACE_TEXT("register BUILT_IN_PUBLICATION_TOPIC_TYPE returned %d.\n"),
                          ret),
                         ret);
      }
    }

    bit_pub_topic_ =
      this->create_topic(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC,
                         OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE,
                         topic_qos,
                         DDS::TopicListener::_nil(),
                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bit_pub_topic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: DomainParticipantImpl::init_bit_topics, ")
                        ACE_TEXT("Nil %C Topic \n"),
                        OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC),
                       DDS::RETCODE_ERROR);
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DomainParticipant::init_bit_topics.");
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::init_bit_subscriber()
{
  try {
    DDS::SubscriberQos sub_qos;
    this->get_default_subscriber_qos(sub_qos);

    bit_subscriber_
    = this->create_subscriber(sub_qos,
                              DDS::SubscriberListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DomainParticipant::create_bit_subscriber.");
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
DomainParticipantImpl::init_bit_datareaders()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  try {
    DDS::TopicDescription_var bit_part_topic_desc
    = this->lookup_topicdescription(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);

    // QoS policies for the DCPSParticipant built-in topic reader.
    DDS::DataReaderQos participantReaderQos;
    bit_subscriber_->get_default_datareader_qos(participantReaderQos);
    participantReaderQos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    if (this->federated_) {
      participantReaderQos.liveliness.lease_duration.nanosec = 0;
      participantReaderQos.liveliness.lease_duration.sec
      = TheServiceParticipant->federation_liveliness();
    }

    DDS::DataReader_var dr
    = bit_subscriber_->create_datareader(bit_part_topic_desc.in(),
                                         participantReaderQos,
                                         DDS::DataReaderListener::_nil(),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bit_part_dr_
    = DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr.in());

    if (this->federated_) {
      // Determine the repository key to which we are attached.
      int key = TheServiceParticipant->domain_to_repo(this->domain_id_);

      // Create and attach the listener.
      this->failoverListener_ = new FailoverListener(key);
      this->bit_part_dr_->set_listener(this->failoverListener_, DEFAULT_STATUS_MASK);
    }

    DDS::DataReaderQos dr_qos;
    bit_subscriber_->get_default_datareader_qos(dr_qos);
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::TopicDescription_var bit_topic_topic_desc
    = this->lookup_topicdescription(OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC);

    dr = bit_subscriber_->create_datareader(bit_topic_topic_desc.in(),
                                            dr_qos,
                                            DDS::DataReaderListener::_nil(),
                                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bit_topic_dr_ =
      DDS::TopicBuiltinTopicDataDataReader::_narrow(dr.in());

    DDS::TopicDescription_var bit_pub_topic_desc =
      this->lookup_topicdescription(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);

    dr = bit_subscriber_->create_datareader(bit_pub_topic_desc.in(),
                                            dr_qos,
                                            DDS::DataReaderListener::_nil(),
                                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bit_pub_dr_ =
      DDS::PublicationBuiltinTopicDataDataReader::_narrow(dr.in());

    DDS::TopicDescription_var bit_sub_topic_desc =
      this->lookup_topicdescription(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);

    dr = bit_subscriber_->create_datareader(bit_sub_topic_desc.in(),
                                            dr_qos,
                                            DDS::DataReaderListener::_nil(),
                                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bit_sub_dr_
    = DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(dr.in());

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DomainParticipant::init_bit_datareaders.");
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
#else

  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

DDS::ReturnCode_t
DomainParticipantImpl::attach_bit_transport()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  try {
    TransportConfig_rch config = TheServiceParticipant->bit_transport_config();

    TransportRegistry::instance()->bind_config(config, bit_subscriber_.in());

  } catch (const OpenDDS::DCPS::Transport::Exception& ex) {
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
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

OwnershipManager*
DomainParticipantImpl::ownership_manager ()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (! CORBA::is_nil (this->bit_pub_dr_.in())) {
    DDS::DataReaderListener_var listener = this->bit_pub_dr_->get_listener ();
    if (CORBA::is_nil (listener.in())) {
      DDS::DataReaderListener_var bit_pub_listener(new BitPubListenerImpl(this));
      this->bit_pub_dr_->set_listener (bit_pub_listener.in (), ::DDS::DATA_AVAILABLE_STATUS);
    }
  }

#endif
  return &this->owner_man_;
}

void
DomainParticipantImpl::update_ownership_strength (const PublicationId& pub_id,
                                                  const CORBA::Long& ownership_strength)
{
  if (this->get_deleted ())
    return;

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

} // namespace DCPS
} // namespace OpenDDS
