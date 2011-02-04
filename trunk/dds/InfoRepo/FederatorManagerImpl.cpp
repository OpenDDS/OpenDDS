/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include "FederatorManagerImpl.h"
#include "DCPSInfo_i.h"
#include "DefaultValues.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "tao/ORB_Core.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include "FederatorTypeSupportC.h"
#include "FederatorTypeSupportImpl.h"

#if !defined (__ACE_INLINE__)
# include "FederatorManagerImpl.inl"
#endif /* !__ACE_INLINE__ */

namespace OpenDDS {
namespace Federator {

ManagerImpl::ManagerImpl(Config& config)
  : joining_(this->lock_),
    joiner_(NIL_REPOSITORY),
    federated_(false),
    config_(config),
    ownerListener_(*this),
    topicListener_(*this),
    participantListener_(*this),
    publicationListener_(*this),
    subscriptionListener_(*this),
    multicastEnabled_(false)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::ManagerImpl()\n")));
  }

  char* mdec = ACE_OS::getenv("MulticastDiscoveryEnabled");

  if (mdec != 0) {
    std::string mde(ACE_OS::getenv("MulticastDiscoveryEnabled"));

    if (mde != "0") {
      multicastEnabled_ = true;
    }
  }
}

ManagerImpl::~ManagerImpl()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::~ManagerImpl()\n")));
  }
}

void
ManagerImpl::initialize()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federation::ManagerImpl::initialize()\n")));
  }

  // Let the listeners know which repository we are to filter samples at
  // the earliest opportunity.
  this->ownerListener_.federationId()        = this->id();
  this->topicListener_.federationId()        = this->id();
  this->participantListener_.federationId()  = this->id();
  this->publicationListener_.federationId()  = this->id();
  this->subscriptionListener_.federationId() = this->id();

  // Add participant for Federation domain
  this->federationParticipant_
  = TheParticipantFactory->create_participant(
      this->config_.federationDomain(),
      PARTICIPANT_QOS_DEFAULT,
      DDS::DomainParticipantListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(this->federationParticipant_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: create_participant failed for ")
               ACE_TEXT("repository %d in federation domain %d.\n"),
               this->id(),
               this->config_.federationDomain()));
    throw Incomplete();
  }

  //
  // Add type support for update topics
  //

  OwnerUpdateTypeSupportImpl* ownerUpdate = new OwnerUpdateTypeSupportImpl();

  if (DDS::RETCODE_OK != ownerUpdate->register_type(
        this->federationParticipant_.in(),
        OWNERUPDATETYPENAME)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to install ")
               ACE_TEXT("OwnerUpdate type support for repository %d.\n"),
               this->id()));
    throw Incomplete();
  }

  ParticipantUpdateTypeSupportImpl* participantUpdate = new ParticipantUpdateTypeSupportImpl();

  if (DDS::RETCODE_OK != participantUpdate->register_type(
        this->federationParticipant_.in(),
        PARTICIPANTUPDATETYPENAME)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to install ")
               ACE_TEXT("ParticipantUpdate type support for repository %d.\n"),
               this->id()));
    throw Incomplete();
  }

  TopicUpdateTypeSupportImpl* topicUpdate = new TopicUpdateTypeSupportImpl();

  if (DDS::RETCODE_OK != topicUpdate->register_type(
        this->federationParticipant_.in(),
        TOPICUPDATETYPENAME)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to install ")
               ACE_TEXT("TopicUpdate type support for repository %d.\n"),
               this->id()));
    throw Incomplete();
  }

  PublicationUpdateTypeSupportImpl* publicationUpdate = new PublicationUpdateTypeSupportImpl();

  if (DDS::RETCODE_OK != publicationUpdate->register_type(
        this->federationParticipant_.in(),
        PUBLICATIONUPDATETYPENAME)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to install ")
               ACE_TEXT("PublicationUpdate type support for repository %d.\n"),
               this->id()));
    throw Incomplete();
  }

  SubscriptionUpdateTypeSupportImpl* subscriptionUpdate = new SubscriptionUpdateTypeSupportImpl();

  if (DDS::RETCODE_OK != subscriptionUpdate->register_type(
        this->federationParticipant_.in(),
        SUBSCRIPTIONUPDATETYPENAME)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to install ")
               ACE_TEXT("SubscriptionUpdate type support for repository %d.\n"),
               this->id()));
    throw Incomplete();
  }

  //
  // Create the transport for the update topic publications.
  //

  OpenDDS::DCPS::TransportImpl_rch transport
  = TheTransportFactory->create_transport_impl(
      this->config_.federationDomain(),
      ACE_TEXT("SimpleTcp"),
      OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch transportConfig
  = TheTransportFactory->create_configuration(
      this->config_.federationDomain(),
      ACE_TEXT("SimpleTcp"));

  if (transport->configure(transportConfig.in()) != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("repository %d failed to initialize subscription transport.\n"),
               this->id()));
    throw Incomplete();
  }

  //
  // Create the transport for the update topic subscriptions.
  //

  OpenDDS::DCPS::TransportImpl_rch subscriptionTransport
  = TheTransportFactory->create_transport_impl(
      1 + this->config_.federationDomain(),
      ACE_TEXT("SimpleTcp"),
      OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch subscriptionTransportConfig
  = TheTransportFactory->create_configuration(
      1 + this->config_.federationDomain(),
      ACE_TEXT("SimpleTcp"));

  if (subscriptionTransport->configure(subscriptionTransportConfig.in()) != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("repository %d failed to initialize subscription transport.\n"),
               this->id()));
    throw Incomplete();
  }

  //
  // Create the subscriber for the update topics.
  //

  DDS::Subscriber_var subscriber
  = this->federationParticipant_->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT,
      DDS::SubscriberListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(subscriber.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create subscriber for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("created federation subscriber for repository %d\n"),
               this->id()));

  }

  // Attach the transport to it.
  OpenDDS::DCPS::SubscriberImpl* subscriberServant
  = dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>(
      subscriber.in());

  if (0 == subscriberServant) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract servant for federation subscriber.\n")));
    throw Incomplete();
  }

  switch (subscriberServant->attach_transport(subscriptionTransport.in())) {
  case OpenDDS::DCPS::ATTACH_OK:

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("attached transport to federation subscriber.\n")));
    }

    break;

  case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
  case OpenDDS::DCPS::ATTACH_ERROR:
  case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
  default:
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to attach transport to federation subscriber.\n")));
    throw Incomplete();
  }

  //
  // Create the publisher for the update topics.
  //

  DDS::Publisher_var publisher
  = this->federationParticipant_->create_publisher(
      PUBLISHER_QOS_DEFAULT,
      DDS::PublisherListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(publisher.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create publisher for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("created federation publisher for repository %d\n"),
               this->id()));

  }

  // Attach the transport to it.
  OpenDDS::DCPS::PublisherImpl* publisherServant
  = dynamic_cast<OpenDDS::DCPS::PublisherImpl*>(
      publisher.in());

  if (0 == publisherServant) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract servant for federation publisher.\n")));
    throw Incomplete();
  }

  switch (publisherServant->attach_transport(transport.in())) {
  case OpenDDS::DCPS::ATTACH_OK:

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("attached transport to federation publisher.\n")));
    }

    break;

  case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
  case OpenDDS::DCPS::ATTACH_ERROR:
  case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
  default:
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to attach transport to federation publisher.\n")));
    throw Incomplete();
  }

  //
  // Some useful items for adding the subscriptions.
  //
  DDS::Topic_var            topic;
  DDS::TopicDescription_var description;
  DDS::DataReader_var       dataReader;
  DDS::DataWriter_var       dataWriter;

  DDS::DataReaderQos readerQos;
  subscriber->get_default_datareader_qos(readerQos);
  readerQos.durability.kind                          = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  readerQos.history.kind                             = DDS::KEEP_LAST_HISTORY_QOS;
  readerQos.history.depth                            = 50;
  readerQos.reliability.kind                         = DDS::RELIABLE_RELIABILITY_QOS;
  readerQos.reliability.max_blocking_time.sec        = 0;
  readerQos.reliability.max_blocking_time.nanosec    = 0;

  DDS::DataWriterQos writerQos;
  publisher->get_default_datawriter_qos(writerQos);
  writerQos.durability.kind                          = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = DDS::KEEP_LAST_HISTORY_QOS;
  writerQos.history.depth                            = 50;
  writerQos.reliability.kind                         = DDS::RELIABLE_RELIABILITY_QOS;
  writerQos.reliability.max_blocking_time.sec        = 0;
  writerQos.reliability.max_blocking_time.nanosec    = 0;

  //
  // Add update subscriptions
  //
  // NOTE: Its ok to lose the references to the objects here since they
  //       are not needed after this point.  The only thing we will do
  //       with them is to destroy them, and that will be done via a
  //       cascade delete from the participant.  The listeners will
  //       survive and can be used within other participants as well,
  //       since the only state they retain is the manager, which is the
  //       same for all.
  //

  topic = this->federationParticipant_->create_topic(
            OWNERUPDATETOPICNAME,
            OWNERUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  dataWriter = publisher->create_datawriter(
                 topic.in(),
                 writerQos,
                 DDS::DataWriterListener::_nil(),
                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataWriter.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create OwnerUpdate writer for repository %d\n"),
               this->id()));
    throw Incomplete();
  }

  this->ownerWriter_
  = OwnerUpdateDataWriter::_narrow(dataWriter.in());

  if (::CORBA::is_nil(this->ownerWriter_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract typed OwnerUpdate writer.\n")));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dataWriter.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed OwnerUpdate writer.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_publication_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation OwnerUpdate writer %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  description = this->federationParticipant_->lookup_topicdescription(OWNERUPDATETOPICNAME);
  dataReader  = subscriber->create_datareader(
                  description.in(),
                  readerQos,
                  &this->ownerListener_,
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataReader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create OwnerUpdate reader for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataReaderImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dataReader.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed OwnerUpdate reader.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_subscription_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation OwnerUpdate reader %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  topic = this->federationParticipant_->create_topic(
            TOPICUPDATETOPICNAME,
            TOPICUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  dataWriter = publisher->create_datawriter(
                 topic.in(),
                 writerQos,
                 DDS::DataWriterListener::_nil(),
                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataWriter.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create TopicUpdate writer for repository %d\n"),
               this->id()));
    throw Incomplete();
  }

  this->topicWriter_
  = TopicUpdateDataWriter::_narrow(dataWriter.in());

  if (::CORBA::is_nil(this->topicWriter_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract typed TopicUpdate writer.\n")));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dataWriter.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed TopicUpdate writer.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_publication_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation TopicUpdate writer %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  description = this->federationParticipant_->lookup_topicdescription(TOPICUPDATETOPICNAME);
  dataReader  = subscriber->create_datareader(
                  description.in(),
                  readerQos,
                  &this->topicListener_,
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataReader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create TopicUpdate reader for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataReaderImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dataReader.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed TopicUpdate reader.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_subscription_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation TopicUpdate reader %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  topic = this->federationParticipant_->create_topic(
            PARTICIPANTUPDATETOPICNAME,
            PARTICIPANTUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  dataWriter = publisher->create_datawriter(
                 topic.in(),
                 writerQos,
                 DDS::DataWriterListener::_nil(),
                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataWriter.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create ParticipantUpdate writer for repository %d\n"),
               this->id()));
    throw Incomplete();
  }

  this->participantWriter_
  = ParticipantUpdateDataWriter::_narrow(dataWriter.in());

  if (::CORBA::is_nil(this->participantWriter_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract typed ParticipantUpdate writer.\n")));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dataWriter.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed ParticipantUpdate writer.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_publication_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation ParticipantUpdate writer %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  description = this->federationParticipant_->lookup_topicdescription(PARTICIPANTUPDATETOPICNAME);
  dataReader  = subscriber->create_datareader(
                  description.in(),
                  readerQos,
                  &this->participantListener_,
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataReader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create ParticipantUpdate reader for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataReaderImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dataReader.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed ParticipantUpdate reader.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_subscription_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation ParticipantUpdate reader %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  topic = this->federationParticipant_->create_topic(
            PUBLICATIONUPDATETOPICNAME,
            PUBLICATIONUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  dataWriter = publisher->create_datawriter(
                 topic.in(),
                 writerQos,
                 DDS::DataWriterListener::_nil(),
                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataWriter.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create PublicationUpdate writer for repository %d\n"),
               this->id()));
    throw Incomplete();
  }

  this->publicationWriter_
  = PublicationUpdateDataWriter::_narrow(dataWriter.in());

  if (::CORBA::is_nil(this->publicationWriter_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract typed PublicationUpdate writer.\n")));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dataWriter.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed PublicationUpdate writer.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_publication_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation PublicationUpdate writer %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  description = this->federationParticipant_->lookup_topicdescription(PUBLICATIONUPDATETOPICNAME);
  dataReader  = subscriber->create_datareader(
                  description.in(),
                  readerQos,
                  &this->publicationListener_,
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataReader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create PublicationUpdate reader for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataReaderImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dataReader.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed PublicationUpdate reader.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_subscription_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation PublicationUpdate reader %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  topic = this->federationParticipant_->create_topic(
            SUBSCRIPTIONUPDATETOPICNAME,
            SUBSCRIPTIONUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  dataWriter = publisher->create_datawriter(
                 topic.in(),
                 writerQos,
                 DDS::DataWriterListener::_nil(),
                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataWriter.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create SubscriptionUpdate writer for repository %d\n"),
               this->id()));
    throw Incomplete();
  }

  this->subscriptionWriter_
  = SubscriptionUpdateDataWriter::_narrow(dataWriter.in());

  if (::CORBA::is_nil(this->subscriptionWriter_.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to extract typed SubscriptionUpdate writer.\n")));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(dataWriter.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed SubscriptionUpdate writer.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_publication_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation SubscriptionUpdate writer %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  description = this->federationParticipant_->lookup_topicdescription(SUBSCRIPTIONUPDATETOPICNAME);
  dataReader  = subscriber->create_datareader(
                  description.in(),
                  readerQos,
                  &this->subscriptionListener_,
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dataReader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::ManagerImpl::initialize() - ")
               ACE_TEXT("failed to create SubscriptionUpdate reader for repository %d\n"),
               this->id()));
    throw Incomplete();

  } else if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::DataReaderImpl* servant
    = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(dataReader.in());

    if (0 == servant) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("unable to extract typed SubscriptionUpdate reader.\n")));

    } else {
      OpenDDS::DCPS::RepoIdConverter converter(servant->get_subscription_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("created federation SubscriptionUpdate reader %C for repository %d\n"),
                 std::string(converter).c_str(),
                 this->id()));
    }
  }

  // JSP
#if defined (ACE_HAS_IP_MULTICAST)

  if (this->multicastEnabled_) {
    //
    // Install ior multicast handler.
    //
    // Get reactor instance from TAO.
    ACE_Reactor *reactor = this->orb_->orb_core()->reactor();

    // See if the -ORBMulticastDiscoveryEndpoint option was specified.
    ACE_CString mde(this->orb_->orb_core()->orb_params()->mcast_discovery_endpoint());

    // First, see if the user has given us a multicast port number
    // on the command-line;
    u_short port = 0;

    // Check environment var. for multicast port.
    const char *port_number = ACE_OS::getenv("OpenDDSFederationPort");

    if (port_number != 0) {
      port = static_cast<u_short>(ACE_OS::atoi(port_number));
    }

    // Port wasn't specified on the command-line -
    // use the default.
    if (port == 0)
      port = OpenDDS::Federator::Defaults::DiscoveryRequestPort;

    // Initialize the handler
    if (mde.length() != 0) {
      if (this->multicastResponder_.init(
            this->orb_.in(),
            mde.c_str()) == -1) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to initialize ")
                   ACE_TEXT("the multicast responder for repository %d.\n"),
                   this->id()));
        throw Incomplete();
      }

    } else {
      if (this->multicastResponder_.init(
            this->orb_.in(),
            port,
#if defined (ACE_HAS_IPV6)
            ACE_DEFAULT_MULTICASTV6_ADDR
#else
            ACE_DEFAULT_MULTICAST_ADDR
#endif /* ACE_HAS_IPV6 */
          )) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Unable to initialize ")
                   ACE_TEXT("the multicast responder for repository %d.\n"),
                   this->id()));
        throw Incomplete();
      }
    }

    // Register event handler for the ior multicast.
    if (reactor->register_handler(&this->multicastResponder_,
                                  ACE_Event_Handler::READ_MASK) == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Unable to register event handler ")
                 ACE_TEXT("for repository %d.\n"),
                 this->id()));
      throw Incomplete();
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::initialize() - ")
                 ACE_TEXT("multicast server setup is complete.\n")));
    }
  }

#else
  ACE_UNUSED_ARG(this->multicastEnabled_);
#endif /* ACE_HAS_IP_MULTICAST */
}

void
ManagerImpl::finalize()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::finalize()\n")));
  }

  ownerListener_.stop();
  topicListener_.stop();
  participantListener_.stop();
  publicationListener_.stop();
  subscriptionListener_.stop();
  ownerListener_.join();
  topicListener_.join();
  participantListener_.join();
  publicationListener_.join();
  subscriptionListener_.join();

  if (this->federated_) {
    try {
      IdToManagerMap::iterator where = this->peers_.find(this->joinRepo_);

      if (where == this->peers_.end()) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Federator::Manager::finalize: ")
                   ACE_TEXT("repository %d - all attachment to federation left.\n"),
                   this->id()));

      } else {
        if (CORBA::is_nil(where->second.in())) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: Federator::Manager::finalize: ")
                     ACE_TEXT("repository %d not currently attached to a federation.\n"),
                     this->id()));

        } else {
          where->second->leave_federation(this->id());
          this->federated_ = false;
        }
      }

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception(
        ACE_TEXT("ERROR: Federator::ManagerImpl::finalize() - ")
        ACE_TEXT("unable to leave remote federation "));
      throw Incomplete();
    }
  }

  if (!CORBA::is_nil(this->orb_.in()) && (0 != this->orb_->orb_core())) {
    this->orb_->orb_core()->reactor()->remove_handler(
      &this->multicastResponder_,
      ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);
  }

  // Remove our local participant and contained entities.
  if (0 == CORBA::is_nil(this->federationParticipant_.in())) {
    if (DDS::RETCODE_PRECONDITION_NOT_MET
        == this->federationParticipant_->delete_contained_entities()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Federator::Manager ")
                 ACE_TEXT("unable to release resources for repository %d.\n"),
                 this->id()));

    } else if (DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant(this->federationParticipant_.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Federator::Manager ")
                 ACE_TEXT("unable to release the participant for repository %d.\n"),
                 this->id()));
    }
  }
}

// IDL methods.

RepoKey
ManagerImpl::federation_id()
ACE_THROW_SPEC((::CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::federation_id()\n")));
  }

  return this->id();
}

OpenDDS::DCPS::DCPSInfo_ptr
ManagerImpl::repository()
ACE_THROW_SPEC((::CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::repository()\n")));
  }

  OpenDDS::DCPS::DCPSInfo_var repo
  = TheServiceParticipant->get_repository(
      this->config_.federationDomain());

  if (CORBA::is_nil(repo.in())) {
    return OpenDDS::DCPS::DCPSInfo::_duplicate(this->localRepo_.in());

  } else {
    return OpenDDS::DCPS::DCPSInfo::_duplicate(repo.in());
  }
}

CORBA::Boolean
ManagerImpl::discover_federation(const char * ior)
ACE_THROW_SPEC((::CORBA::SystemException, Incomplete))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::discover_federation( %C)\n"),
               ior));
  }

  ///@TODO: Implement this.
  return false;
}

Manager_ptr
ManagerImpl::join_federation(
  Manager_ptr peer,
  FederationDomain federation
) ACE_THROW_SPEC((::CORBA::SystemException, Incomplete))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::join_federation( peer, %d)\n"),
               federation));
  }

  RepoKey remote = NIL_REPOSITORY;

  try {
    // Obtain the remote repository federator Id value.
    remote = peer->federation_id();

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::join_federation() - ")
                 ACE_TEXT("repo id %d entered from repository with id %d.\n"),
                 this->id(),
                 remote));
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      ACE_TEXT("ERROR: Federator::ManagerImpl::join_federation() - ")
      ACE_TEXT("unable to obtain remote federation Id value: "));
    throw Incomplete();
  }

  // If we are recursing, then we are done.
  if (this->joiner_ == remote) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::join_federation() - ")
                 ACE_TEXT("repo id %d leaving after reentry from repository with id %d.\n"),
                 this->id(),
                 remote));
    }

    return this->_this();

  } else {
    // Block while any different repository is joining.
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);

    while (this->joiner_ != NIL_REPOSITORY) {
      // This releases the lock while we block.
      this->joining_.wait();

      // We are now recursing - curses!
      if (this->joiner_ == remote) {
        return this->_this();
      }
    }

    // Note that we are joining the remote repository now.
    this->joiner_ = remote;
  }

  //
  // We only reach this point if:
  //   1) No other repository is processing past this point;
  //   2) We are not recursing.
  //

  // Check if we already have Federation repository.
  // Check if we are already federated.
  if (this->federated_ == false) {
    // Go ahead and add the joining repository as our Federation
    // repository.
    try {
      // Mark this repository as the point to which we are joined to
      // the federation.
      this->joinRepo_ = remote;

      // Obtain a reference to the remote repository.
      OpenDDS::DCPS::DCPSInfo_var remoteRepo = peer->repository();

      if (OpenDDS::DCPS::DCPS_debug_level > 4) {
        CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
        CORBA::String_var remoteRepoIor = orb->object_to_string(remoteRepo.in());
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) FederatorManagerImpl::join_federation() - ")
                   ACE_TEXT("id %d obtained reference to id %d:\n")
                   ACE_TEXT("\t%C\n"),
                   this->id(),
                   remote,
                   remoteRepoIor.in()));
      }

      // Add remote repository to Service_Participant in the Federation domain
      TheServiceParticipant->set_repo(remoteRepo.in(), remote);
      TheServiceParticipant->set_repo_domain(this->config_.federationDomain(), remote);

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception(
        "ERROR: Federator::ManagerImpl::join_federation() - Unable to join with remote: ");
      throw Incomplete();
    }
  }

  // Symmetrical joining behavior.
  try {
    Manager_var remoteManager
    = peer->join_federation(this->_this(), this->config_.federationDomain());

    if (this->joinRepo_ == remote) {
      this->peers_[ this->joinRepo_]
      = OpenDDS::Federator::Manager::_duplicate(remoteManager.in());
    }

    //
    // Push our initial state out to the joining repository *after* we call
    // him back to join.  This reduces the amount of duplicate data pushed
    // when a new (empty) repository is joining an existing federation.
    //
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Federator::ManagerImpl::join_federation() - ")
                 ACE_TEXT("repo id %d pushing state to repository with id %d.\n"),
                 this->id(),
                 remote));
    }

    this->pushState(peer);

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Federator::ManagerImpl::join_federation() - unsuccsessful call to remote->join: ");
    throw Incomplete();
  }

  if (CORBA::is_nil(this->participantWriter_.in())) {
    //
    // Establish our update publications and subscriptions *after* we
    // have exhanged internal state with the first joining repository.
    //
    this->initialize();
  }

  // Adjust our joining state and give others the opportunity to proceed.
  if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Federator::ManagerImpl::join_federation() - ")
               ACE_TEXT("repo id %d joined to repository with id %d.\n"),
               this->id(),
               remote));
  }

  this->federated_ = true;
  this->joiner_    = NIL_REPOSITORY;
  this->joining_.signal();
  return this->_this();
}

void
ManagerImpl::leave_federation(
  RepoKey id)
ACE_THROW_SPEC((CORBA::SystemException,
                 Incomplete))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::leave_federation( %d)\n"),
               this->id()));
  }

  // Remove the leaving repository from our outbound mappings.
  IdToManagerMap::iterator where = this->peers_.find(id);

  if (where != this->peers_.end()) {
    this->peers_.erase(where);
  }

  // Remove all the internal Entities owned by the leaving repository.
  if (false
      == this->info_->remove_by_owner(this->config_.federationDomain(), id)) {
    throw Incomplete();
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ManagerImpl::leave_federation( %d) complete.\n"),
               this->id()));
  }
}

void
ManagerImpl::leave_and_shutdown(
  void)
ACE_THROW_SPEC((CORBA::SystemException,
                 Incomplete))
{
  // Shutdown the process via the repository object.
  this->info_->shutdown();
}

void
ManagerImpl::shutdown(
  void)
ACE_THROW_SPEC((CORBA::SystemException,
                 Incomplete))
{
  // Prevent the removal of this repository from the federation during
  // shutdown processing.
  this->federated_ = false;

  // Shutdown the process via the repository object.
  this->info_->shutdown();
}

void
ManagerImpl::initializeOwner(
  const OpenDDS::Federator::OwnerUpdate & data)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::Federator::Incomplete))
{
  this->processCreate(&data, 0);
}

void
ManagerImpl::initializeTopic(
  const OpenDDS::Federator::TopicUpdate & data)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::Federator::Incomplete))
{
  this->processCreate(&data, 0);
}

void
ManagerImpl::initializeParticipant(
  const OpenDDS::Federator::ParticipantUpdate & data)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::Federator::Incomplete))
{
  this->processCreate(&data, 0);
}

void
ManagerImpl::initializePublication(
  const OpenDDS::Federator::PublicationUpdate & data)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::Federator::Incomplete))
{
  this->processCreate(&data, 0);
}

void
ManagerImpl::initializeSubscription(
  const OpenDDS::Federator::SubscriptionUpdate & data)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::Federator::Incomplete))
{
  this->processCreate(&data, 0);
}

} // namespace Federator
} // namespace OpenDDS
