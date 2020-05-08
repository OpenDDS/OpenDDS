
#include "Writer.h"
#include <ace/OS_NS_unistd.h>
#include <iostream>

using namespace Messenger;
using namespace std;

static int s_i_msg_cnt = 5;
static int s_i_subject = 99;

Writer::Writer (::DDS::DataWriter_ptr writer)
  : writer_ (::DDS::DataWriter::_duplicate (writer))
  , timeout_writes_ (0)
  , count_ (1)
  , dwl_servant_ (0)
{
  ::DDS::DataWriterListener_var dwl = writer->get_listener ();
  this->dwl_servant_ =
    dynamic_cast<DataWriterListenerImpl*> (dwl.in ());
}

Writer::~Writer ()
{
}


//------------------------------------------------------------------------------
int
Writer::svc ()
{
  if (this->dwl_servant_ == 0)
    return -1;  // Should not occur.

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%P|%t) Writer::svc begins.\n")));

  int i_subject;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->lock_, -1);
    i_subject = s_i_subject++;
  }

  this->write_loop(i_subject, NULL, NULL, NULL, s_i_msg_cnt);

  ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  return 0;
}

//------------------------------------------------------------------------------
int
Writer::write_loop(int i_subject, char const *pc_from, char const *pc_subj, char const *pc_text, int i_msgs)
{
  static char const s_ac_from[] = "Comic Book Guy";
  static char const s_ac_subj[] = "Review";
  static char const s_ac_text[] = "Worst. Movie. Ever.";

  if (NULL == pc_from)  pc_from = s_ac_from;
  if (NULL == pc_subj)  pc_subj = s_ac_subj;
  if (NULL == pc_text)  pc_text = s_ac_text;

  try
  {
    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow (writer_.in ());

    if (CORBA::is_nil (message_dw.in ()))
    {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = i_subject;
    message.count      = 0;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    printf("--pub-- write   hdl %2u  subj %3d  count %3d   new\n", handle, message.subject_id, message.count);
    fflush(stdout);

    message.from    = CORBA::string_dup(pc_from);
    message.subject = CORBA::string_dup(pc_subj);
    message.text    = CORBA::string_dup(pc_text);

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT ("(%P|%t) %T Writer::svc starting to write.\n")));

    for (int i_msg = 0; i_msg < i_msgs; ++i_msg)
    {
      int i_count;
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->lock_, -1);
        i_count = this->count_++;
      }

      ::DDS::ReturnCode_t i_rc = this->write_one(message_dw.in (), handle, message, i_count);

      if (i_rc != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT ("%dth write_one() returned %d.\n"),
                    i_msg, i_rc));
      }
    }
  }
  catch (CORBA::Exception& e)
  {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  return 0;
}

//------------------------------------------------------------------------------
int Writer::set_count(int i_count)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->lock_, -1);
  this->count_ = i_count;
  return 0;
}

//------------------------------------------------------------------------------
bool
Writer::start (int i_threads, int i_msg_cnt, int i_subject_1st)
{
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) Starting Writer \n")));
  s_i_msg_cnt = i_msg_cnt;
  s_i_subject = i_subject_1st;

  // Launch threads.
  if (this->activate (THR_NEW_LWP | THR_JOINABLE, i_threads) == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%P|%t) %p\n"),
                       ACE_TEXT ("Error activating threads.\n")),
                      false);
  }

  return true;
}

bool
Writer::end ()
{
  int const result = this->wait ();

  if (result != 0)
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("(%P|%t) %p\n"),
                ACE_TEXT ("Error waiting for threads.\n")));
  else
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("(%P|%t) Done writing. \n")));

  return result == 0;
}

int
Writer::get_timeout_writes () const
{
  return timeout_writes_.value ();
}

//------------------------------------------------------------------------------
::DDS::ReturnCode_t
Writer::write_one(Messenger::MessageDataWriter_ptr message_dw, ::DDS::InstanceHandle_t& handle, Messenger::Message& message, int i_count)
{
  message.count = i_count;

  printf("--pub-- write   hdl %2u  subj %3d  count %3d   write\n", handle, message.subject_id, message.count);
  fflush(stdout);

  ::DDS::ReturnCode_t const ret = message_dw->write (message, handle);

  if (ret == ::DDS::RETCODE_TIMEOUT)
  {
    ++this->timeout_writes_;
  }

  return ret;
}
