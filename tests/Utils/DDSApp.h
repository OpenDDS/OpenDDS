/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_DDSApp_H
#define TestUtils_DDSApp_H

#include "TestUtils_Export.h"

#include "tests/Utils/DDSTopicFacade.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"

#include <map>
#include <string>
#include <stdexcept>

namespace TestUtils {

/// Application to represent all of DDS to the rest of an application that
/// wants to send and receive DDS messages.  The class will create or use
/// default parameters when they are not provided.
///
/// DDSTopicFacade: This is the essential piece of DDS functionality, it is used
/// to create data readers and data writers for a particular topic.  A
/// DDSTopicFacade will be created for a participant and a topic.  If the
/// participant is not provided, it is set to the default participant (see
/// below) and the topic is created in DDS when the DDSTopicFacade is created.
///
/// domain_id: The domain_id is either provided when explicitly creating a
/// participant, or the default_domain_id_ is used.  The
/// default_domain_id_ will be set if it is explicitly set in the
/// constructor or it will be set the the first domain_id explicitly passed
/// when creating a participant (or else it will be 0).
///
/// participant: A participant can either be provided explicitly when
/// creating a DDSTopicFacade (by first calling participant(...)) or it will be
/// set to the default_participant_.  The default_participant_ is set to
/// the first participant that was created (either by calling
/// participant(...) explicitly or by calling topic(...) and not providing
/// a participant).
class TestUtils_Export DDSApp
{
public:
  typedef std::map<DDS::DomainParticipant_ptr, DDS::DomainParticipant_var> Participants;

  /// create a DDSApp with the provided command line (and default domain_id)
  DDSApp(int& argc, ACE_TCHAR**& argv);
  DDSApp(int& argc, ACE_TCHAR**& argv, DDS::DomainId_t default_domain_id);
  ~DDSApp();

  /// create a new participant
  DDS::DomainParticipant_var participant();
  DDS::DomainParticipant_var participant(DDS::DomainId_t                    domain_id);
  template<typename QosFunc>
  DDS::DomainParticipant_var participant(QosFunc                              qos_func);
  template<typename QosFunc>
  DDS::DomainParticipant_var participant(DDS::DomainId_t                    domain_id,
                                         QosFunc                              qos_func,
                                         DDS::DomainParticipantListener_var listener =
                                           DDS::DomainParticipantListener::_nil(),
                                         DDS::StatusMask                    mask =
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DomainParticipant_var participant(DDS::DomainId_t                    domain_id,
                                         DDS::DomainParticipantListener_var listener,
                                         DDS::StatusMask                    mask =
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  /// create a new publisher on the provided participant to be used
  /// explicitly later for creating data writer(s)
  DDS::Publisher_var  publisher(DDS::DomainParticipant_var participant =
                                  DDS::DomainParticipant_var(),
                                DDS::PublisherListener_var a_listener =
                                  DDS::PublisherListener::_nil(),
                                DDS::StatusMask            mask =
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  template<typename QosFunc>
  DDS::Publisher_var  publisher(DDS::DomainParticipant_var   participant,
                                QosFunc                      qos_func,
                                DDS::PublisherListener_var a_listener =
                                  DDS::PublisherListener::_nil(),
                                DDS::StatusMask            mask =
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  /// create a new subscriber on the provided participant to be used
  /// explicitly later for creating data reader(s)
  DDS::Subscriber_var subscriber(DDS::DomainParticipant_var participant =
                                   DDS::DomainParticipant_var(),
                                DDS::SubscriberListener_var a_listener =
                                  DDS::SubscriberListener::_nil(),
                                DDS::StatusMask            mask =
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  template<typename QosFunc>
  DDS::Subscriber_var  subscriber(DDS::DomainParticipant_var  participant,
                                  QosFunc                     qos_func,
                                  DDS::SubscriberListener_var a_listener =
                                    DDS::SubscriberListener::_nil(),
                                  DDS::StatusMask             mask =
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  /// create a new DDSTopicFacade for the given type and topic name
  /// overloaded methods allow optionally providing an already
  /// created participant, setting the qos using a function pointer
  /// or a functor, providing a Topic listener, and/or setting
  /// a non-default status mask.
  /// To provide qos_func it should either be set to the address of a
  /// function or else it should be an object that provides the
  /// implementation:
  /// void operator()(DDS::TopicQos& qos)
  template<typename Traits>
  DDSTopicFacade<Traits> topic_facade(
    std::string topic_name,
    DDS::DomainParticipant_var participant = DDS::DomainParticipant_var(),
    DDS::TopicListener_var listener = DDS::TopicListener::_nil(),
    DDS::StatusMask mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    determine_participant(participant);
    return create_topic_facade<Traits>(topic_name,
                                                    participant,
                                                    TOPIC_QOS_DEFAULT,
                                                    listener,
                                                    mask);
  }

  template<typename Traits, typename QosFunc>
  DDSTopicFacade<Traits> topic_facade(
    std::string topic_name,
    DDS::DomainParticipant_var participant,
    QosFunc qos_func,
    DDS::TopicListener_var listener = DDS::TopicListener::_nil(),
    DDS::StatusMask mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    DDS::TopicQos qos;
    participant->get_default_topic_qos(qos);
    qos_func(qos);
    return create_topic_facade<Traits>(topic_name,
                                                    participant,
                                                    qos,
                                                    listener,
                                                    mask);
  }

  template<typename MessageType, typename QosFunc>
  DDSTopicFacade<MessageType> topic_facade(
    std::string topic_name,
    QosFunc qos_func,
    DDS::TopicListener_var listener = DDS::TopicListener::_nil(),
    DDS::StatusMask mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    DDS::DomainParticipant_var participant;
    determine_participant(participant);
    DDS::TopicQos qos;
    participant->get_default_topic_qos(qos);
    qos_func(qos);
    return create_topic_facade<MessageType>(topic_name,
                                            participant,
                                            qos,
                                            listener,
                                            mask);
  }

  /// cleans up the specified participant, or default participant
  void cleanup(DDS::DomainParticipant_var participant = DDS::DomainParticipant_var());

  /// helper method to downcast a DataWriter
  template<typename Writer_var>
  static DDS::DataWriter_var datawriter(const Writer_var& writer)
  {
    return DDS::DataWriter::_duplicate(writer.in());
  }

  /// cleanup all participants and shutdown dds (DDSApp will no longer be usable)
  void shutdown();

private:
  DDSApp(const DDSApp& rhs);
  DDSApp& operator=(DDSApp& rhs);

  void add(const DDS::DomainParticipant_var& participant);
  void remove(const DDS::DomainParticipant_var& participant);
  void determine_participant(DDS::DomainParticipant_var& participant);
  void assign_default_domain_id(DDS::DomainId_t id);
  DDS::DomainParticipant_var create_part(DDS::DomainId_t                    domain_id,
                                         const DDS::DomainParticipantQos&   qos,
                                         DDS::DomainParticipantListener_var listener,
                                         DDS::StatusMask                    mask);
  DDS::Publisher_var create_pub(DDS::DomainParticipant_var participant,
                                const DDS::PublisherQos&   qos,
                                DDS::PublisherListener_var a_listener,
                                DDS::StatusMask            mask);
  DDS::Subscriber_var create_sub(DDS::DomainParticipant_var  participant,
                                 const DDS::SubscriberQos&   qos,
                                 DDS::SubscriberListener_var a_listener,
                                 DDS::StatusMask             mask);

  template<typename MessageType>
  DDSTopicFacade<MessageType> create_topic_facade(
    std::string topic_name,
    DDS::DomainParticipant_var participant,
    const DDS::TopicQos& qos,
    DDS::TopicListener_var listener,
    DDS::StatusMask mask)
  {
    typedef OpenDDS::DCPS::DDSTraits<MessageType> TraitsType;
    typedef typename TraitsType::TypeSupportType::_var_type TypeSupportVarType;

    TypeSupportVarType ts(new typename TraitsType::TypeSupportTypeImpl);
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error(" ERROR: register_type failed!");
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic(topic_name.c_str(),
                                type_name.in(),
                                qos,
                                listener,
                                mask);

    if (CORBA::is_nil(topic.in())) {
      std::string message = "ERROR: could not create topic \"";
      message += topic_name;
      message += "\" of type \"";
      message += type_name;
      message += "\"!";
      throw std::runtime_error(message);
    }

    return DDSTopicFacade<MessageType>(participant, topic);
  }

  // track the default status for domain id
  bool domain_id_defaulted_;
  DDS::DomainId_t default_domain_id_;

  const DDS::DomainParticipantFactory_var dpf_;

  DDS::DomainParticipant_var default_participant_;
  Participants participants_;
  bool shutdown_;
};

template<typename QosFunc>
DDS::DomainParticipant_var
DDSApp::participant(QosFunc              qos_func)
{
  return participant(default_domain_id_,
                     qos_func,
                     DDS::DomainParticipantListener::_nil(),
                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

template<typename QosFunc>
DDS::DomainParticipant_var
DDSApp::participant(DDS::DomainId_t                    domain_id,
                    QosFunc                            qos_func,
                    DDS::DomainParticipantListener_var listener,
                    DDS::StatusMask                    mask)
{
  DDS::DomainParticipantQos part_qos;
  dpf_->get_default_participant_qos(part_qos);
  qos_func(part_qos);
  return create_part(domain_id,
                     part_qos,
                     listener,
                     mask);
}

template<typename QosFunc>
DDS::Publisher_var
DDSApp::publisher(DDS::DomainParticipant_var participant,
                  QosFunc                    qos_func,
                  DDS::PublisherListener_var a_listener,
                  DDS::StatusMask            mask)
{
  determine_participant(participant);
  DDS::PublisherQos qos;
  participant->get_default_publisher_qos(qos);
  qos_func(qos);
  return create_pub(participant,
                    qos,
                    a_listener,
                    mask);
}

template<typename QosFunc>
DDS::Subscriber_var
DDSApp::subscriber(DDS::DomainParticipant_var  participant,
                   QosFunc                     qos_func,
                   DDS::SubscriberListener_var a_listener,
                   DDS::StatusMask             mask)
{
  determine_participant(participant);
  DDS::SubscriberQos qos;
  participant->get_default_subscriber_qos(qos);
  qos_func(qos);
  return create_sub(participant,
                    qos,
                    a_listener,
                    mask);
}

} // End namespaces

#endif /* TestUtils_DDSApp_H */
