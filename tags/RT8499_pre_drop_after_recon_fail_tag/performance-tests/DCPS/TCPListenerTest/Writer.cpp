// -*- C++ -*-
//
// $Id$
#include "Writer.h"
//#include "TestException.h"
#include "../TypeNoKeyBounded/Pt128TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt128TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/OS_NS_unistd.h"



extern ACE_Condition<ACE_Recursive_Thread_Mutex> done_condition_;



template<class T, class W, class W_var, class W_ptr, class Wimpl>
void write (long id,
            int size,
            int num_messages,
            ::DDS::DataWriter_ptr writer)
{
  T data;
  data.data_source = id;
  data.values.length(size);

  for (int i = 0; i < size; i ++)
  {
    data.values[i] = (float) i;
  }

  W_var pt_dw 
    = W::_narrow(writer ACE_ENV_ARG_PARAMETER);
  ACE_ASSERT (! CORBA::is_nil (pt_dw.in ()));

  Wimpl* pt_servant =
    ::TAO::DCPS::reference_to_servant< Wimpl, W_ptr>
            (pt_dw.in ()  ACE_ENV_SINGLE_ARG_PARAMETER);

  //SHH remove this kludge when the transport is fixed.
  ACE_OS::sleep(2); // ensure that the connection has been fully established
  ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("%T (%P|%t) Writer::svc starting to write.\n")));

  ::DDS::InstanceHandle_t handle 
      = pt_dw->_cxx_register (data ACE_ENV_ARG_PARAMETER);

  {  // Extra scope for VC6
  for (int i = 0; i < num_messages; i ++)
  {
    data.sequence_num = i;
    pt_servant->write(data, 
                      handle 
                      ACE_ENV_ARG_PARAMETER);
    ACE_TRY_CHECK;
  }
  }
}




Writer::Writer(::DDS::DataWriter_ptr writer, 
               int num_messages,
               int data_size, 
               int num_readers,
               int writer_id)
               : writer_ (::DDS::DataWriter::_duplicate (writer)),
  num_messages_ (num_messages),
  data_size_ (data_size),
  num_floats_per_sample_ (data_size),
  num_readers_(num_readers),
  writer_id_ (writer_id),
  finished_sending_ (false)
{
}

void 
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" %P|%t Writer::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1) 
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT(" %P|%t Writer::start, ")
                ACE_TEXT ("%p."), 
                "activate")); 
  }
}

void 
Writer::end () 
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" %P|%t Writer::end \n")));
  wait ();
}


int 
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT(" %P|%t Writer::svc begins samples with %d floats.\n"),
              data_size_));

  ACE_TRY_NEW_ENV
  {
    int num_connected_subs = 0;
    ::DDS::InstanceHandleSeq handles;
    while (num_connected_subs != num_readers_)
      {
        ACE_OS::sleep(1);
        writer_->get_matched_subscriptions(handles);
        num_connected_subs = handles.length();
      }

    switch ( data_size_ )
    {

    case 128:
      {
        write < ::Xyz::Pt128,
               ::Mine::Pt128DataWriter,
               ::Mine::Pt128DataWriter_var,
               ::Mine::Pt128DataWriter_ptr,
               ::Mine::Pt128DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_.in ());
      }
      break;

    case 512:
      {
        write < ::Xyz::Pt512,
               ::Mine::Pt512DataWriter,
               ::Mine::Pt512DataWriter_var,
               ::Mine::Pt512DataWriter_ptr,
               ::Mine::Pt512DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_.in ());
      }
      break;

    case 2048:
      {
        write < ::Xyz::Pt2048,
               ::Mine::Pt2048DataWriter,
               ::Mine::Pt2048DataWriter_var,
               ::Mine::Pt2048DataWriter_ptr,
               ::Mine::Pt2048DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_.in ());
      }
      break;

    case 8192:
      {
        write < ::Xyz::Pt8192,
               ::Mine::Pt8192DataWriter,
               ::Mine::Pt8192DataWriter_var,
               ::Mine::Pt8192DataWriter_ptr,
               ::Mine::Pt8192DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_.in ());
      }
      break;

    default:
      ACE_DEBUG((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
      return 1;
      break;
    };


  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in svc:");
  }
  ACE_ENDTRY;

  ::DDS::InstanceHandleSeq handles;
  while (!finished_sending_)
    {
      ACE_OS::sleep(1);
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        {
          finished_sending_ = true;
          done_condition_.signal(); // tell publisher look if I am finished.
        }
    }

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT(" %P|%t Writer::svc finished.\n")));
  return 0;
}

long 
Writer::writer_id () const
{
  return writer_id_;
}


bool
Writer::is_finished () const
{
  return finished_sending_;
}

