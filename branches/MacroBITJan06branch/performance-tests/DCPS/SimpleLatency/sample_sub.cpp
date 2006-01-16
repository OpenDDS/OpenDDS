//========================================================
/**
 *
 *  @file sample_pub.cpp
 *
 *  @Id: sample_pub.cpp v1.1
 *
 */
//========================================================

#include "SubListener.h"
#include "PubMessageTypeSupportImpl.h"
#include "AckMessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration_rch.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#include <dds/DCPS/transport/simpleUDP/SimpleUdpFactory.h>
#include <dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration_rch.h>
#include <dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration.h>
#include <ace/streams.h>

#include "ace/Get_Opt.h"
#include "ace/Sched_Params.h"

using namespace DDS;
using namespace CORBA;
using namespace DDSPerfTest;



/* void set_rt() */
/*      Attempt to set the real time priority and lock memory */
void set_rt() 
{
  ACE_Sched_Params params(ACE_SCHED_FIFO,
                          ACE_DEFAULT_THREAD_PRIORITY,
                          ACE_SCOPE_PROCESS);

#if defined (ACE_HAS_WTHREADS)
  params.priority(THREAD_PRIORITY_HIGHEST);
#else
  params.priority(20);
#endif

  if (-1 == ACE_OS::sched_params(params))
  {
    ACE_DEBUG ((LM_DEBUG, "WARNING: %p\n"));
  }

#if (defined (MCL_CURRENT) && defined(MCL_FUTURE))
  if (mlockall(MCL_CURRENT || MCL_FUTURE)) {
    ACE_DEBUG ((LM_DEBUG, "WARNING: Could not lock memory - Run with root access.\n"));
  }
#endif
}



/* Global Variables */

const TAO::DCPS::TransportFactory::IdType UDP_TYPE_ID = 10;
const TAO::DCPS::TransportFactory::IdType UDP_IMPL_ID = 10;
const TAO::DCPS::TransportFactory::IdType TCP_TYPE_ID = 20;
const TAO::DCPS::TransportFactory::IdType TCP_IMPL_ID = 20;


int main(int argc, char *argv[])
{
       bool useTCP = true;
       DomainId_t myDomain = 411;

       setbuf (stdout, NULL);

       ACE_Get_Opt get_opts (argc, argv, ACE_LIB_TEXT("u"));

       int ich;
       while ((ich = get_opts ()) != EOF) {
        switch (ich) {
          case 'u': /* u specifies that UDP should be used */
            useTCP = false;
            break;

          default: /* no parameters */
          break;

        }
       }

       /* Try to set realtime scheduling class*/
       set_rt();


       /* Create participant */
       DDS::DomainParticipantFactory_var dpf =
         TheParticipantFactoryWithArgs (argc, argv);

       DDS::DomainParticipant_var dp =
              dpf->create_participant (myDomain,
                                       PARTICIPANT_QOS_DEFAULT, 
                                       DDS::DomainParticipantListener::_nil ());
       if (CORBA::is_nil (dp.in ()) ) {
         cout << argv[0] << "SAMPLE_SUB: ERROR - Create participant failed." << endl;
         exit (1);
       }
       
       /* Create publisher */
       DDS::Publisher_var p =
         dp->create_publisher (PUBLISHER_QOS_DEFAULT,
                               DDS::PublisherListener::_nil ());
       
       /* Initialize the transport for publisher*/
       TAO::DCPS::TransportImpl_rch pub_tcp_impl;
       if (useTCP) {
         TheTransportFactory->register_type (TCP_TYPE_ID,
                                             new TAO::DCPS::SimpleTcpFactory ());
         TAO::DCPS::SimpleTcpConfiguration_rch writer_config =
           new TAO::DCPS::SimpleTcpConfiguration ();
         pub_tcp_impl = TheTransportFactory->create (TCP_IMPL_ID, TCP_TYPE_ID);
         pub_tcp_impl->configure (writer_config.in ());
       } else {
         TheTransportFactory->register_type (UDP_TYPE_ID,
                                             new TAO::DCPS::SimpleUdpFactory ());
         TAO::DCPS::SimpleUdpConfiguration_rch writer_config =
           new TAO::DCPS::SimpleUdpConfiguration ();
         pub_tcp_impl = TheTransportFactory->create (UDP_IMPL_ID, UDP_TYPE_ID);
         writer_config->local_address_.set("localhost:12367");
         pub_tcp_impl->configure (writer_config.in ());
       }

       /* Attach the transport protocol with the publishing entity */
       TAO::DCPS::PublisherImpl* p_impl =
         reference_to_servant <TAO::DCPS::PublisherImpl, DDS::Publisher_ptr> (p);
       p_impl->attach_transport (pub_tcp_impl.in ());



       /* Create topic for datawriter */
       AckMessageTypeSupportImpl* ackmessage_dt = new AckMessageTypeSupportImpl;
       ackmessage_dt->register_type (dp.in (), 
                                    "DDSPerfTest::AckMessage");
       DDS::Topic_var ackmessage_topic = dp->create_topic ("ackmessage_topic", // topic name
                                                           "DDSPerfTest::AckMessage", // topic type
                                                           TOPIC_QOS_DEFAULT, 
                                                           DDS::TopicListener::_nil ());

       /* Create PubMessage data writer */
       DDS::DataWriter_var dw = p->create_datawriter (ackmessage_topic.in (),
                                                      DATAWRITER_QOS_DEFAULT,
                                                      DDS::DataWriterListener::_nil ());
       AckMessageDataWriter_var ackmessage_writer = 
         AckMessageDataWriter::_narrow (dw);
       

       /* Create the subscriber */ 
       DDS::Subscriber_var s =
         dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                               DDS::SubscriberListener::_nil());


       /* Initialize the transport for subscriber */
       TAO::DCPS::TransportImpl_rch sub_tcp_impl;
       if (useTCP) {
         TAO::DCPS::SimpleTcpConfiguration_rch reader_config =
           new TAO::DCPS::SimpleTcpConfiguration();
         sub_tcp_impl = TheTransportFactory->create(TCP_IMPL_ID+1, TCP_TYPE_ID);
         sub_tcp_impl->configure(reader_config.in());
       } else {
         TAO::DCPS::SimpleUdpConfiguration_rch reader_config =
           new TAO::DCPS::SimpleUdpConfiguration();
         sub_tcp_impl = TheTransportFactory->create(UDP_IMPL_ID+1, UDP_TYPE_ID);
         reader_config->local_address_.set("localhost:1237");
         sub_tcp_impl->configure(reader_config.in());
       }

       /* Attach the transport protocol with the subscribing entity */
       TAO::DCPS::SubscriberImpl* sub_impl =
         reference_to_servant< TAO::DCPS::SubscriberImpl,
                               DDS::Subscriber_ptr> (s.in ());
       sub_impl->attach_transport(sub_tcp_impl.in());


       /* Create topic for datareader */
       PubMessageTypeSupportImpl* pubmessage_dt = new PubMessageTypeSupportImpl;
       pubmessage_dt->register_type (dp.in (), 
                                     "DDSPerfTest::PubMessage");
       DDS::Topic_var pubmessage_topic = dp->create_topic ("pubmessage_topic", // topic name
                                                           "DDSPerfTest::PubMessage", // topic type
                                                           TOPIC_QOS_DEFAULT, 
                                                           DDS::TopicListener::_nil ());

       /* Create the listener for datareader */
       PubDataReaderListenerImpl  listener_servant;
       PortableServer::POA_var poa = TheServiceParticipant->the_poa ();
       CORBA::Object_var obj = poa->servant_to_reference(&listener_servant);
       DDS::DataReaderListener_var listener =
         DDS::DataReaderListener::_narrow (obj.in ());


       /* Create AckMessage datareader */
       DDS::DataReader_var dr = s->create_datareader (pubmessage_topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      listener.in ());

       listener_servant.init(dr.in(), dw.in());

       PubMessageDataReader_var pubmessage_reader = 
         PubMessageDataReader::_narrow (dr);

       while (listener_servant.done () == 0) 
       {
         ACE_OS::sleep (1);
       };



       std::cout << "Sub: shut down" << std::endl;
       /* Shut down domain entities */
       dp->delete_contained_entities ();
       dpf->delete_participant (dp.in ());
       TheTransportFactory->release ();
       TheServiceParticipant->shutdown ();
              
       return(0);

}
