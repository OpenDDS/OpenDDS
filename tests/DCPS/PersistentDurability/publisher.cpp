// -*- C++ -*-

// ============================================================================
/**
 *  @file   publisher.cpp
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "DataWriterListenerImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/FileSystemStorage.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/unique_ptr.h"

#include <dds/DCPS/DataWriterImpl_T.h>
#include "MessengerTypeSupportImpl.h"

#include <ace/Atomic_Op_T.h>
#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>

#include <memory>
#include <stdexcept>

using namespace Messenger;

typedef OpenDDS::DCPS::DataWriterImpl_T<Messenger::Message> Message_DataWriterImplType;

static int const s_i_writer_threads  = 2;
static int const s_i_writer_msgs     = 5;
static int const s_i_writer_subj_1st = 99;

bool do_write = false, delete_data = false;
const ACE_TCHAR *dir = ACE_TEXT("");

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("wd:"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'w':
        do_write = true;
        break;

      case 'd':
        delete_data = true;
        dir = get_opts.opt_arg ();
        break;

      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "[-w] [-d directory]"
                           "\n",
                           argv [0]),
                          -1);
      }

  return 0;
}

//------------------------------------------------------------------------------
static void local_show_instances(::DDS::DataWriter *pst_writer_gen)
{
  static char const *s_pc_init = "(init)";

  Message_DataWriterImplType *pst_writer_impl = dynamic_cast<Message_DataWriterImplType*>(pst_writer_gen);

  if (NULL == pst_writer_impl)
  {
    cerr << "Data Writer could not be narrowed to ImplType"<< endl;
    exit(1);
  }

  OpenDDS::DCPS::DataWriterImpl::InstanceHandleVec st_vec_inst;

  st_vec_inst.clear();
  pst_writer_impl->get_instance_handles(st_vec_inst);

  printf("--pub-- Check Instances = %2u\n", (uint32_t)st_vec_inst.size());
  fflush(stdout);

  uint32_t ui32_ndx = 0;
  for (OpenDDS::DCPS::DataWriterImpl::InstanceHandleVec::iterator iter = st_vec_inst.begin();  iter != st_vec_inst.end();  ++iter, ++ui32_ndx)
  {
    Messenger::Message      st_msg;
    ::DDS::InstanceHandle_t st_hdl = *iter;
    size_t                  siz_samples = 0;

    st_msg.from       = s_pc_init;
    st_msg.subject    = s_pc_init;
    st_msg.text       = s_pc_init;
    st_msg.subject_id = 0;
    st_msg.count      = 0;
    st_msg.ull        = 0;
    st_msg.source_pid = 0;

    pst_writer_impl->num_samples(st_hdl, siz_samples);

    pst_writer_impl->get_key_value(st_msg, st_hdl);

    printf("--pub-- ndx %2u  hdl %2u  samps %2u      subj_id: %3d  \n",
      ui32_ndx, st_hdl, (uint32_t)siz_samples, st_msg.subject_id);
    fflush(stdout);

    // printf("--pub-- ndx %2u  hdl %2u  samps %2u      subj_id: %3d  cnt: %2d  from: %-16s  subj: %-10s  text: %-20s\n",
      // ui32_ndx, st_hdl, (uint32_t)siz_samples, st_msg.subject_id, st_msg.count,
      // st_msg.from.in(), st_msg.subject.in(), st_msg.text.in());
    // fflush(stdout);
  }
}

//------------------------------------------------------------------------------
static void local_find_instances(::DDS::DataWriter *pst_writer_gen, int i_subj_1st, int i_subj_cnt)
{
  Message_DataWriterImplType *pst_writer_impl = dynamic_cast<Message_DataWriterImplType*>(pst_writer_gen);

  if (NULL == pst_writer_impl)
  {
    cerr << "Data Writer could not be narrowed to ImplType"<< endl;
    exit(1);
  }

  ::DDS::InstanceHandle_t st_hdl;
  Messenger::Message      st_msg;

  for (int i_subj = i_subj_1st;  i_subj < i_subj_1st + i_subj_cnt;  ++i_subj)
  {
    st_msg.subject_id = i_subj;
    st_hdl = pst_writer_impl->lookup_instance(st_msg);
    printf("--pub-- Find key %3d = hdl %2u\n", i_subj, st_hdl);
    fflush(stdout);
  }
}

//------------------------------------------------------------------------------
int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs (argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant (111,
                                 PARTICIPANT_QOS_DEFAULT,
                                 DDS::DomainParticipantListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ()))
      {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      if (parse_args (argc, argv) != 0)
        return -1;

      if (delete_data)
      {
        using OpenDDS::FileSystemStorage::Directory;
        Directory::create (ACE_TEXT_ALWAYS_CHAR (dir))->remove ();
        dpf->delete_participant (participant);
        TheServiceParticipant->shutdown ();
        return 0;
      }

      MessageTypeSupport_var servant = new MessageTypeSupportImpl ();

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), ""))
      {
        cerr << "register_type failed." << endl;
        exit (1);
      }

      CORBA::String_var type_name = servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos (topic_qos);
      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        cerr << "create_topic failed." << endl;
        exit (1);
      }

      DDS::Publisher_var pub =
        participant->create_publisher (PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        cerr << "create_publisher failed." << endl;
        exit (1);
      }

      // Configure DataWriter QoS policies.
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      dw_qos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
      dw_qos.durability_service.history_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.resource_limits.max_samples_per_instance = 1000;
      dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;

      // -------------------------------------------------------

      {
        DataWriterListenerImpl* listener = new DataWriterListenerImpl;
        DDS::DataWriterListener_var dwl = listener;

        // Create a DataWriter.

        // Upon exiting this scope, all unsent data should be
        // transferred to OpenDDS's data durability cache since the
        // run_test.pl script should not have started the subscriber
        // until it detects the "Done writing" log text.
        DDS::DataWriter_var dw =
          pub->create_datawriter (topic.in (),
                                  dw_qos,
                                  dwl.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dw.in ()))
        {
          cerr << "create_datawriter failed." << endl;
          exit (1);
        }

        printf("--pub------- After create_datawriter()\n");
        fflush(stdout);
        local_show_instances(dw.in());
        local_find_instances(dw.in(), s_i_writer_subj_1st, s_i_writer_threads);

        // Only write samples if configured to do so.  The expectation
        // is to otherwise retrieve the data from the PERSISTENT data
        // durability cache.
        if (do_write)
        {
          // Write samples.
          OpenDDS::DCPS::unique_ptr<Writer> writer (new Writer (dw.in ()));

          printf("--pub------- Run   writer threads\n");
          fflush(stdout);
          if (!writer->start(s_i_writer_threads, s_i_writer_msgs, s_i_writer_subj_1st) || !writer->end())
          {
            // Error logging performed in above method call.
            exit (1);
          }

          printf("--pub------- After writer threads\n");
          fflush(stdout);
          local_show_instances(dw.in());
          local_find_instances(dw.in(), s_i_writer_subj_1st, s_i_writer_threads);

          // Explicitly destroy the DataWriter.
          if (pub->delete_datawriter (dw.in ())
              == ::DDS::RETCODE_PRECONDITION_NOT_MET)
          {
            cerr << "Unable to delete DataWriter" << endl;
            exit (1);
          }
          printf("--pub------- After delete_datawriter()\n");
          fflush(stdout);
        }
        else
        {
          int const max_attempts = 50;
          int attempts;
          for (attempts = 1;
               attempts != max_attempts
                 && listener->publication_matched_.value () == false;
               ++attempts)
          {
            ACE_OS::sleep (5);
          }

          if (attempts == max_attempts)
          {
            cerr << "ERROR: subscriptions failed to match." << endl;
            exit (1);
          }

          OpenDDS::DCPS::unique_ptr<Writer> writer (new Writer (dw.in ()));

          printf("--pub------- Call  write new subject\n");
          fflush(stdout);
          writer->write_loop(s_i_writer_subj_1st + s_i_writer_threads, "Main", "One more", "Nothing here", 1);
          printf("--pub------- After write new subject\n");
          fflush(stdout);
          local_show_instances(dw.in());
          local_find_instances(dw.in(), s_i_writer_subj_1st, s_i_writer_threads+1);

          printf("--pub------- Call  write 1st subject\n");
          fflush(stdout);
          writer->set_count(s_i_writer_threads * s_i_writer_msgs - 1);
          writer->write_loop(s_i_writer_subj_1st, NULL, NULL, NULL, 1);
          printf("--pub------- After write 1st subject\n");
          fflush(stdout);
          local_show_instances(dw.in());
          local_find_instances(dw.in(), s_i_writer_subj_1st, s_i_writer_threads+1);

          printf("--pub------- Wait for DataReader to finish\n");
          fflush(stdout);
          // Wait for DataReader to finish.
          ::DDS::InstanceHandleSeq handles;
          for (attempts = 1; attempts != max_attempts; ++attempts)
          {
            dw->get_matched_subscriptions (handles);
            if (handles.length () == 0)
              break;
            else
              ACE_OS::sleep(1);
          }

          // The data durability cache should no longer contain samples
          // for this domain/topic/type.
        }
      }

      // -------------------------------------------------------

      {
        // Write samples that will not be sent.  Exercise
        // service_cleanup_delay.  We can either do this through the
        // durability member or durability_service member in either of
        // TopicQos or DataWriterQos.  This test arbitrarily uses the
        // DataWriterQos::durability_service member.

        // Cleanup data after this number of seconds.
        CORBA::Long const delay_seconds = 5;

        ::DDS::Duration_t & cleanup_delay =
          dw_qos.durability_service.service_cleanup_delay;
        cleanup_delay.sec     = delay_seconds;
        cleanup_delay.nanosec = 0;

        // Create a dummy topic which will have no subscriptions.
        DDS::Topic_var dummy_topic =
          participant->create_topic ("Dummy Topic",
                                     type_name.in (),
                                     topic_qos,
                                     DDS::TopicListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        DDS::DataWriter_var dummy_dw =
          pub->create_datawriter (dummy_topic.in (),
                                  dw_qos,
                                  ::DDS::DataWriterListener::_nil (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dummy_dw.in ()))
        {
          cerr << "create_datawriter for dummy topic failed." << endl;
          exit (1);
        }

        printf("--pub------- Test service cleanup\n");
        fflush(stdout);
        // Write samples using multiple threads.
        OpenDDS::DCPS::unique_ptr<Writer> writer (new Writer (dummy_dw.in ()));

        // Explicitly destroy the DataWriter.
        if (pub->delete_datawriter (dummy_dw.in ())
            == ::DDS::RETCODE_PRECONDITION_NOT_MET)
        {
          cerr << "Unable to delete DataWriter" << endl;
          exit (1);
        }

        printf("--pub------- Sleep %d\n", delay_seconds + 3);
        fflush(stdout);
        // Allow durability cleanup to occur
        ACE_OS::sleep (delay_seconds + 3);
      }

      printf("--pub------- Delete and shutdown ...\n");
      fflush(stdout);
      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheServiceParticipant->shutdown ();
  } catch (CORBA::Exception& e) {
     cerr << "PUB: Exception caught in main.cpp:" << endl
          << e << endl;
      exit (1);
    }
  catch (const std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
                      err.what()), -1);
  }

  return 0;
}
