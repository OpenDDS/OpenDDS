#include "AbstractionLayer.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/BuiltInTopicUtils.h>

#include "FileInfoListener.h"
#include "ApplicationLevel.h"

AbstractionLayer::AbstractionLayer()
: handle_(0)
, application_(0)
{
}


AbstractionLayer::~AbstractionLayer()
{
}


bool
AbstractionLayer::init_DDS(int& argc, ACE_TCHAR *argv[])
{
  // Initialize the Participant Factory
  dpf_ = TheParticipantFactoryWithArgs (argc, argv);
  if (CORBA::is_nil (dpf_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create participant factory failed.\n") ));
    return false;
  }


  // Create participant
  dp_ = dpf_->create_participant (DOMAINID,
                                  PARTICIPANT_QOS_DEFAULT,
                                  DDS::DomainParticipantListener::_nil (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (dp_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create participant failed.\n") ));
    return false;
  }

  // Create publisher
  pub_ = dp_->create_publisher (PUBLISHER_QOS_DEFAULT,
                                DDS::PublisherListener::_nil (),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (pub_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create publisher failed.\n") ));
    return false;
  }


  // Create topic for datawriter and datareader
  DistributedContent::FileDiffTypeSupportImpl* fileinfo_dt =
                new DistributedContent::FileDiffTypeSupportImpl();
  fileinfo_dt->register_type (dp_.in (),
                              "DistributedContent::FileDiff");
   topic_ = dp_->create_topic ("fileinfo_topic", // topic name
                               "DistributedContent::FileDiff", // topic type
                               TOPIC_QOS_DEFAULT,
                               DDS::TopicListener::_nil (),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (topic_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create topic failed.\n") ));
    return false;
  }


  // Create the subscriber
  sub_ = dp_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                DDS::SubscriberListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (sub_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create subscriber failed.\n") ));
    return false;
  }

  // Create the listener for datareader
  listener_ = new FileInfoListener(this);


  // Create the datareader
  dr_ = sub_->create_datareader (topic_.in (),
                                 DATAREADER_QOS_DEFAULT,
                                 listener_.in (),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (dr_.in ()) ) {
    ACE_ERROR((LM_ERROR, "ERROR - Create data reader failed.\n"));
    return false;
  }


  // Setup the ignores so the data writer for this node will not associate with
  //  the data reader of this node.

  // We need the servants of the Domain Participant and the Data Reader to
  // get information to set up the ignore.
  ::OpenDDS::DCPS::DomainParticipantImpl* dp_servant =
    dynamic_cast< ::OpenDDS::DCPS::DomainParticipantImpl*>(dp_.in());
  if (0 == dp_servant ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Servant dereference of domain participant failed.\n") ));
    return false;
  }

  ::OpenDDS::DCPS::DataReaderImpl* dr_servant =
    dynamic_cast< ::OpenDDS::DCPS::DataReaderImpl*>(dr_.in());
  if (0 == dr_servant ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Servant dereference of data reader failed.\n") ));
    return false;
  }

  // Get the repo id for the subscription
  ::OpenDDS::DCPS::GUID_t ignore_id = dr_servant->get_guid ();
  // Get the instance handle for the subscription
  ::DDS::InstanceHandle_t handle = dp_servant->lookup_handle(ignore_id);

  // tell the domain participant to ignore the subscription
  DDS::ReturnCode_t ret = dp_->ignore_subscription (handle);
  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("ERROR - Failed to ignore_publication %d return error %d\n"),
                handle, ret));
    return false;
  }


  // Create the datawriter
  dw_ = pub_->create_datawriter (topic_.in (),
                                 DATAWRITER_QOS_DEFAULT,
                                 DDS::DataWriterListener::_nil (),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (dw_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Create data writer failed.\n") ));
    return false;
  }

  // Narrow down the data writer to the FileDiffDataWriter
  filediff_writer_ = DistributedContent::FileDiffDataWriter::_narrow (dw_.in());
  if (CORBA::is_nil (filediff_writer_.in ()) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Narrow of data writer failed.\n") ));
    return false;
  }

  return true;
}



void
AbstractionLayer::shutdown_DDS()
{

  // Clean up all the entities in the participant
  dp_->delete_contained_entities ();

  // Clean up the participant
  dpf_->delete_participant (dp_.in ());

  // Clean up the transport

  // Clean up DDS
  TheServiceParticipant->shutdown ();
}




void
AbstractionLayer::attach_application(ApplicationLevel* app)
{
  application_ = app;
}


void
AbstractionLayer::receive_diff(const DistributedContent::FileDiff& diff)
{
  // At this the point, the memory could be copied to create a true abstraction.


  // Pass the diff to the application if it is defined.
  if (0 != application_)
  {
    application_->receive_diff(diff);
  }
  else
  {
    ACE_DEBUG((LM_DEBUG,
      "Warning: AbstractionLayer received diff while no application defined.\n"));
  }
}


bool
AbstractionLayer::send_diff(const DistributedContent::FileDiff& diff)
{
  // Have the writer publish the diff
  DDS::ReturnCode_t result = filediff_writer_->write(diff, handle_);

  return (result == DDS::RETCODE_OK);
}
