// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================

#include "SyncClientExt_i.h"

#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/scoped_ptr.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/High_Res_Timer.h"
#include "ace/OS_NS_unistd.h"

#include <sstream>

class Subscriber
{
public:
  typedef std::string InitError;

  Subscriber (int argc, ACE_TCHAR *argv[]) throw (InitError);

  bool run ();

private:
  bool parse_args (int argc, ACE_TCHAR *argv[]);

  size_t topic_count_;
  size_t participant_count_;
  size_t reader_count_;

  std::string control_file_;
  size_t publisher_count_;

  std::string sync_server_;

  DDS::DomainParticipantFactory_var dpf_;
  std::vector<DDS::DomainParticipant_var> participant_;
  std::vector<DDS::Topic_var> topic_;
  std::vector<DDS::Subscriber_var> subs_;
  std::vector<DDS::DataReader_var> dr_;

  OpenDDS::DCPS::scoped_ptr<SyncClientExt_i> sync_client_;
};


bool
Subscriber::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("t:n:p:c:s:i:"));
  int c;
  std::string usage = " -t <topic count>\n"
    " -n <participant count>\n -p <publisher count>\n"
    " -c <control file>\n -s <subscriber count>\n"
    " -y <SyncServer ior>";

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
        publisher_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'c':
        control_file_ = ACE_TEXT_ALWAYS_CHAR (get_opts.opt_arg ());
        break;
      case 's':
        reader_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 'y':
        sync_server_ = ACE_TEXT_ALWAYS_CHAR (get_opts.opt_arg ());
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

Subscriber::Subscriber (int argc, ACE_TCHAR *argv[]) throw (Subscriber::InitError)
  : topic_count_ (1), participant_count_ (1), reader_count_(1)
  , control_file_ ("barrier_file"), publisher_count_ (1)
{
  try
    {
      dpf_ = TheParticipantFactoryWithArgs (argc, argv);

      if (!this->parse_args (argc, argv)) {
        throw InitError ("Subscriber::ctor> Failed to parse args.");
      }

      sync_client_.reset (new SyncClientExt_i (sync_server_, CORBA::ORB::_nil()
                                            , SyncClient_i::Sub));
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

  size_t max_subs
    = ((participant_count_ > topic_count_) ? topic_count_ : participant_count_);
  max_subs = ((max_subs > reader_count_) ? reader_count_ : max_subs);
  reader_count_ = max_subs;

  participant_.resize (participant_count_);
  topic_.resize (topic_count_);
  subs_.resize (reader_count_);
  dr_.resize (reader_count_);
}

bool
Subscriber::run (void)
{
  ::DDS::DomainId_t domain_id = 411;

  try
    {
      sync_client_->way_point_reached (1);
      sync_client_->get_notification ();

      /*
        size_t max_wait_time = 10; //(seconds)
        size_t wait_time = 0;
        while (true)
        {
        if (wait_time > max_wait_time) {
        std::cerr << "Timed out waiting for external file: "
        << control_file_.c_str() << std::endl;
        return -1;
        }

        // check for file
        ACE_stat my_stat;
        if (ACE_OS::stat (control_file_.c_str(), &my_stat) == 0) {
        // found the trigger file.
        break;
        }

        ACE_OS::sleep (1); wait_time++;
        }
      */

      ACE_High_Res_Timer participant_timer;
      participant_timer.start();
      for (size_t count = 0; count < participant_count_; count++)
        {
          participant_[count] =
            dpf_->create_participant (domain_id,
                                      PARTICIPANT_QOS_DEFAULT,
                                      DDS::DomainParticipantListener::_nil(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (participant_[count].in ())) {
            cerr << "create_participant failed." << endl;
            return false;
          }
        }
      participant_timer.stop();

      Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl();

      for (size_t count = 0; count < participant_count_; count++)
        {
          if (DDS::RETCODE_OK != mts
              ->register_type(participant_[count].in (), "")) {
            cerr << "register_type failed." << endl;
            return false;
          }
        }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::TopicQos topic_qos;

      ACE_High_Res_Timer topic_timer;
      topic_timer.start();
      for (size_t count = 0; count < topic_count_; count++)
        {
          size_t part_count = ((count < participant_count_) ? count: 0);
          participant_[part_count]->get_default_topic_qos(topic_qos);
          topic_[count] =
            participant_[part_count]->create_topic ("Movie Discussion List",
                                                    type_name.in (),
                                                    topic_qos,
                                                    DDS::TopicListener::_nil(),
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (topic_[count].in ())) {
            cerr << "create_topic failed." << endl;
            return false;
          }
        }
      topic_timer.stop();

      ACE_High_Res_Timer sub_timer;
      sub_timer.start();
      for (size_t count = 0; count < reader_count_; count++)
        {
          // Create the subscriber
          subs_[count] =
            participant_[count]->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                   DDS::SubscriberListener::_nil(),
                                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (subs_[count].in ())) {
            cerr << "Failed to create_subscriber." << endl;
            return false;
          }
        }

      // Create the Datareaders
      DDS::DataReaderQos dr_qos;
      subs_[0]->get_default_datareader_qos (dr_qos);

      for (size_t count = 0; count < reader_count_; count++)
        {
          dr_[count]
            = subs_[count]->create_datareader(topic_[count].in (),
                                     dr_qos,
                                     DDS::DataReaderListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (dr_[count].in ())) {
            cerr << "create_datareader failed." << endl;
            return false;
          }
        }

      // Wait for all expected subscribers
      while (true)
        {
          ::DDS::InstanceHandleSeq handles;
          dr_[0]->get_matched_publications(handles);
          //ACE_DEBUG ((LM_DEBUG, "pubs connected: %d\n", handles.length()));
          if (handles.length() >= publisher_count_) {
            break;
          }

          ACE_OS::sleep (1);
        }
      sub_timer.stop ();

      // sync up
      sync_client_->way_point_reached (2);
      sync_client_->get_notification ();

      ACE_Time_Value tv;
      participant_timer.elapsed_time (tv);
      sync_client_->publish(SyncExt::Topic, static_cast<int>(topic_count_), tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d participants in %d secs.\n"
      //, participant_count_, tv.sec()));

      topic_timer.elapsed_time (tv);
      sync_client_->publish(SyncExt::Participant, static_cast<int>(participant_count_), tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d topics in %d secs.\n"
      //, topic_count_, tv.sec()));

      sub_timer.elapsed_time (tv);
      sync_client_->publish(SyncExt::Subscriber, static_cast<int>(reader_count_), tv.msec());
      //ACE_DEBUG ((LM_DEBUG, "(%P|%t) Created %d subscribers in %d secs.\n"
      //, reader_count_, tv.sec()));


      for (size_t count = 0; count < participant_count_; count++)
        {
          participant_[count]->delete_contained_entities ();
          dpf_->delete_participant (participant_[count].in ());
        }
      //ACE_OS::sleep(2);

      TheServiceParticipant->shutdown ();

    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in Subscriber::run():\n  "
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
      Subscriber subscriber (argc, argv);

      subscriber.run ();
    }
  catch (Subscriber::InitError& ex)
    {
      std::string& msg = reinterpret_cast<std::string&>(ex);

      std::cerr << "Unexpected initialization Error: "
                << msg.c_str() << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: Subscriber caught exception");

      return -1;
    }

  return 0;
}
