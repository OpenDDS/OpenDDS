/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_DDSApp_H
#define TestUtils_DDSApp_H

#include "TestUtils_Export.h"

#include "tests/Utils/DDSTopic.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"

#include <model/Sync.h>
#include <map>
#include <string>
#include <stdexcept>

namespace TestUtils {

/// Application to represent all of DDS to the rest of an application that
/// wants to send and receive DDS messages.  The class will create or use
/// default parameters when they are not provided.
///
/// DDSTopic: This is the essential piece of DDS functionality, it is used
/// to create data readers and data writers for a particular topic.  A
/// DDSTopic will be created for a participant and a topic.  If the
/// participant is not provided, it is set to the default participant (see
/// below) and the topic is created in DDS when the DDSTopic is created.
///
/// domain_id: The domain_id is either provided when explicitly creating a
/// participant, or the default_domain_id_ is used.  The
/// default_domain_id_ will be set if it is explicitly set in the
/// constructor or it will be set the the first domain_id explicitly passed
/// when creating a participant (or else it will be 0).
///
/// participant: A participant can either be provided explicitly when
/// creating a DDSTopic (by first calling participant(...)) or it will be
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

  /// create a new DDSTopic for the given type and topic name
  template<typename WriterOrReaderImpl>
  DDSTopic<WriterOrReaderImpl> topic(
    std::string topic_name,
    DDS::DomainParticipant_var participant = DDS::DomainParticipant_var())
  {
    typedef typename WriterOrReaderImpl::typesupportimpl_type typesupportimpl_type;
    typedef typename typesupportimpl_type::typesupport_var typesupport_var;

    // call this first to ensure the dpf is created first if we are doing lazy
    // initialization
    domain_participant_factory();
    determine_participant(participant);

    typesupport_var ts(new typesupportimpl_type);
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error(" ERROR: register_type failed!");
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic(topic_name.c_str(),
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      std::string message = "ERROR: could not create topic \"";
      message += topic_name;
      message += "\" of type \"";
      message += type_name;
      message += "\"!";
      throw std::runtime_error(message);
    }

    return DDSTopic<WriterOrReaderImpl>(participant, topic);
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

  DDS::DomainParticipantFactory_var domain_participant_factory();
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

  int& argc_;
  ACE_TCHAR**& argv_;

  // track the default status for domain id
  bool domain_id_defaulted_;
  DDS::DomainId_t default_domain_id_;

  // don't use this var, use domain_participant_factory() to allow lazy initialization
  DDS::DomainParticipantFactory_var dpf_dont_use_;

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
  DDS::DomainParticipantFactory_var dpf = domain_participant_factory();
  DDS::DomainParticipantQos part_qos;
  dpf->get_default_participant_qos(part_qos);
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
