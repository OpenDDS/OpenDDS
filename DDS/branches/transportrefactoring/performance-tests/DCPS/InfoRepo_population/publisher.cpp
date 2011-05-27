// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "SyncClientExt_i.h"

#include "MessageTypeSupportImpl.h"
#include "Writer.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include <ace/streams.h>
#include "ace/High_Res_Timer.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_sys_stat.h"

#include <string>

class Publisher
{
public:
  typedef std::string InitError;

  Publisher (int argc, char *argv[]) throw (InitError);

  bool run ();

private:
  bool parse_args (int argc, char *argv[]);

  size_t topic_count_;
  size_t participant_count_;
  size_t writer_count_;

  std::string control_file_;
  size_t subscriber_count_;

  int transport_impl_id_;

  std::string sync_server_;

  DDS::DomainParticipantFactory_var dpf_;
  std::vector<DDS::DomainParticipant_var> participant_;
  std::vector<DDS::Topic_var> topic_;
  std::vector<DDS::Publisher_var> pub_;
  std::vector<DDS::DataWriter_var> dw_;

  std::auto_ptr<SyncClientExt_i> sync_client_;
};

bool
Publisher::parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "t:n:p:c:s:i:");
  int c;
  std::string usage = " -t <topic count>\n"
    " -n <participant count>\n -p <publisher count>\n"
    " -c <control file>\n -s <subscriber count>\n"
    " -i <transport Id>\n -y <syncServer ior>";

  while ((c = get_opts ()) != -1)
  {
    switch (c)
      {
      case 't':
        topic_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'n':
        participant_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'p':
        writer_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'c':
        control_file_ = get_opts.opt_arg ();
        break;
      case 's':
        subscriber_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'i':
        transport_impl_id_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'y':
        sync_server_ = get_opts.opt_arg ();
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage> %s\n",
                           usage.c_str()),
                          false);
      }
  }

  // Indicates successful parsing of the command line
  return true;
}

Publisher::Publisher (int argc, char *argv[]) throw (InitError)
  : topic_count_ (1), participant_count_ (1), writer_count_ (1)
  , control_file_ ("barrier_file"), subscriber_count_(1)
  , transport_impl_id_ (1)
{
  try
    {
      dpf_ = TheParticipantFactoryWithArgs (argc, argv);

      if (!this->parse_args (argc, argv)) {
        throw InitError ("Publisher::ctor> Failed to parse args.");
      }

      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Publisher> Creating SyncClient_i\n"));
      sync_client_.reset (new SyncClientExt_i (sync_server_, CORBA::ORB::_nil()
                                            , SyncClient_i::Pub));
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Publisher> SyncClient_i created\n"));
    }
  catch (SyncClient_i::InitError& er)
    {
      std::cerr << "Exception in SyncClient_i initialization."
                << std::endl;
      throw InitError (er);
    }
  catch (CORBA::Exception& ex)
    {
       cerr << "PUB: Exception caught in Publisher cstr:" << endl
            << ex << endl;
       throw InitError (ex._info().c_str());
    }

  size_t max_pubs
    = ((participant_count_ > topic_count_) ? topic_count_ : participant_count_);
  max_pubs = ((max_pubs > writer_count_) ? writer_count_ : max_pubs);
  writer_count_ = max_pubs;

  participant_.resize (participant_count_);
  topic_.resize (topic_count_);
  pub_.resize (writer_count_);
  dw_.resize (writer_count_);
}

bool
Publisher::run (void)
{
  ::DDS::DomainId_t domain_id = 411;

  try
    {
      sync_client_->way_point_reached (1);
      sync_client_->get_notification ();

      ACE_High_Res_Timer participant_timer;
      participant_timer.start();
      for (size_t count = 0; count < participant_count_; count++)
        {
          participant_[count] =
            dpf_->create_participant (domain_id,
                                      PARTICIPANT_QOS_DEFAULT,
                                      DDS::DomainParticipantListener::_nil());
          if (CORBA::is_nil (participant_[count].in ())) {
            cerr << "create_participant failed." << endl;
            return false;
          }
        }
      participant_timer.stop();

      Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl();

      for (size_t count = 0; count < participant_count_; count++)
        {
          if (DDS::RETCODE_OK != mts->register_type(participant_[count].in (), "")) {
            cerr << "register_type failed." << endl;
            return false;
          }
        }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::TopicQos topic_qos;
      participant_[0]->get_default_topic_qos(topic_qos);

      ACE_High_Res_Timer topic_timer;
      topic_timer.start();
      for (size_t count = 0; count < topic_count_; count++)
        {
          topic_[count] =
            participant_[count%participant_count_]->create_topic ("Movie Discussion List",
                                                                  type_name.in (),
                                                                  topic_qos,
                                                                  DDS::TopicListener::_nil());
          if (CORBA::is_nil (topic_[count].in ())) {
            cerr << "create_topic failed." << endl;
            return false;
          }
        }
      topic_timer.stop();

      // Initialize the transport
      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id_,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);
      ACE_High_Res_Timer pub_timer;
      pub_timer.start();
      for (size_t count = 0; count < writer_count_; count++)
        {
          // Create the publisher and attach to the corresponding
          // transport.
          pub_[count] =
            participant_[count]->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                  DDS::PublisherListener::_nil());

          if (CORBA::is_nil (pub_[count].in ())) {
            cerr << "create_publisher failed." << endl;
            return false;
          }

          OpenDDS::DCPS::PublisherImpl* pub_impl =
            ::OpenDDS::DCPS::reference_to_servant< OpenDDS::DCPS::PublisherImpl,
            DDS::Publisher_ptr>(pub_[count].in ());
          if (0 == pub_impl) {
            cerr << "Failed to obtain publisher servant" << endl;
            return false;
          }

          // Attach the publisher to the transport.
          OpenDDS::DCPS::AttachStatus status = pub_impl->attach_transport(tcp_impl.in());
          if (status != OpenDDS::DCPS::ATTACH_OK)
            {
              std::string status_str;
              switch (status) {
              case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = "ATTACH_BAD_TRANSPORT";
                break;
              case OpenDDS::DCPS::ATTACH_ERROR:
                status_str = "ATTACH_ERROR";
                break;
              case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = "ATTACH_INCOMPATIBLE_QOS";
                break;
              default:
                status_str = "Unknown Status";
                break;
              }
              cerr << "Failed to attach to the transport. Status == "
                   << status_str.c_str() << endl;
              return false;
            }
        }

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub_[0]->get_default_datawriter_qos (dw_qos);

      for (size_t count = 0; count < writer_count_; count++)
        {
          dw_[count] =
            pub_[count]->create_datawriter(topic_[count].in (),
                                   dw_qos,
                                   DDS::DataWriterListener::_nil());
          if (CORBA::is_nil (dw_[count].in ())) {
            cerr << "create_datawriter failed." << endl;
            return false;
          }
        }

      // Wait for all expected subscribers
      while (true)
        {
          ::DDS::InstanceHandleSeq handles;
          dw_[0]->get_matched_subscriptions(handles);
          //ACE_DEBUG ((LM_DEBUG, "(%P|%t) subs connected: %d\n", handles.length()));
          if (handles.length() >= subscriber_count_) {
            break;
          }
          ACE_OS::sleep (1);
        }
      pub_timer.stop ();

      // sync up
      sync_client_->way_point_reached (2);
      sync_client_->get_notification ();


      ACE_Time_Value tv;
      participant_timer.elapsed_time (tv);
      sync_client_->publish (SyncExt::Topic, topic_count_, tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d participants in %d secs.\n"
      //, participant_count_, tv.sec()));

      topic_timer.elapsed_time (tv);
      sync_client_->publish (SyncExt::Participant, participant_count_
                             , tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d topics in %d secs.\n"
      //, topic_count_, tv.sec()));

      pub_timer.elapsed_time (tv);
      sync_client_->publish (SyncExt::Publisher, writer_count_
                             , tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d publishers in %d secs.\n"
      //, writer_count_, tv.sec()));


      /*
        std::auto_ptr<Writer> writer (new Writer(dw_[0].in()));

        writer->start ();
        while ( !writer->is_finished()) {
        ACE_Time_Value small(0,250000);
        ACE_OS::sleep (small);
        }

        // Cleanup
        writer->end ();
        //delete writer;
        */

      for (size_t count = 0; count < participant_count_; count++)
        {
          participant_[count]->delete_contained_entities ();
          dpf_->delete_participant (participant_[count].in ());
        }
      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
      cerr << "PUB: Exception caught in Publisher::run():\n  "
           << e << endl;
       return false;
    }

  return true;
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  try
    {
      Publisher publisher (argc, argv);

      publisher.run ();
    }
  catch (Publisher::InitError& ex)
    {
      std::string& msg = reinterpret_cast<std::string&>(ex);

      std::cerr << "Initialization Error: "
                << msg.c_str() << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: Publisher caught exception");

      return -1;
    }

  return 0;
}
