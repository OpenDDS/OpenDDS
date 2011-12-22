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
#include "GuidConverter.h"
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
    pub_id_gen_(dp_id_)
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
                               pub_id_gen_.next(),
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

  RepoId ignoreId = get_repoid(handle);
  HandleMap::const_iterator location = this->ignored_participants_.find(ignoreId);

  if (location == this->ignored_participants_.end()) {
    this->ignored_participants_[ ignoreId] = handle;
  }
  else {// ignore same participant again, just return ok.
    return DDS::RETCODE_OK;
  }

  try {
    if (DCPS_debug_level >= 4) {
      GuidConverter converter(dp_id_);
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
      GuidConverter converter(dp_id_);
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

  RepoId ignoreId = get_repoid(handle);
  HandleMap::const_iterator location = this->ignored_topics_.find(ignoreId);

  if (location == this->ignored_topics_.end()) {
    this->ignored_topics_[ ignoreId] = handle;
  }
  else { // ignore same topic again, just return ok.
    return DDS::RETCODE_OK;
  }

  try {
    if (DCPS_debug_level >= 4) {
      GuidConverter converter(dp_id_);
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

  try {
    if (DCPS_debug_level >= 4) {
      GuidConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_publication: ")
                 ACE_TEXT("%C ignoring handle %x.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    RepoId ignoreId = get_repoid(handle);
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

  try {
    if (DCPS_debug_level >= 4) {
      GuidConverter converter(dp_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DomainParticipantImpl::ignore_subscription: ")
                 ACE_TEXT("%C ignoring handle %d.\n"),
                 std::string(converter).c_str(),
                 handle));
    }

    
    RepoId ignoreId = get_repoid(handle);
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
    Discovery_rch disc = TheServiceParticipant->get_discovery(this->domain_id_);
    this->bit_subscriber_ = disc->init_bit(this);
  }

  return ret;
}

RepoId
DomainParticipantImpl::get_id()
{
  return dp_id_;
}

std::string
DomainParticipantImpl::get_unique_id()
{
  return GuidConverter(dp_id_).uniqueId();
}


DDS::InstanceHandle_t
DomainParticipantImpl::get_instance_handle()
ACE_THROW_SPEC((CORBA::SystemException))
{
  return this->get_handle(this->dp_id_);
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
  DDS::InstanceHandle_t result;

  if (location == this->handles_.end()) {
    // Map new handle in both directions
    result = this->participant_handles_.next();
    this->handles_[ id] = result;
    this->repoIds_[ result] = id;
  } else {
    result = location->second;
  }

  return result;
}

RepoId
DomainParticipantImpl::get_repoid(const DDS::InstanceHandle_t& handle)
{
  RepoId result = GUID_UNKNOWN;
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->handle_protector_,
                   GUID_UNKNOWN);
  RepoIdMap::const_iterator location = this->repoIds_.find(handle);
  if (location != this->repoIds_.end()) {
    result = location->second;
  }
  return result;
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
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantImpl::create_topic_i, ")
                 ACE_TEXT("can't create a Topic: type_name \"%C\"")
                 ACE_TEXT("is not registered.\n"), type_name));
    }

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

  if ((enabled_ == true)
      && (qos_.entity_factory.autoenable_created_entities == 1)) {
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

DomainParticipantImpl::RepoIdSequence::RepoIdSequence(RepoId& base) :
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


} // namespace DCPS
} // namespace OpenDDS
