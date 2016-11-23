// -*- C++ -*-
//

#include "Reader.h"
#include "TestException.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Serializer.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportC.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportImpl.h"
#include "ace/OS_NS_unistd.h"

template<class Tseq, class R, class R_var, class R_ptr>
::DDS::ReturnCode_t read (TestStats* stats,
                          ::DDS::Subscriber_ptr subscriber,
                          ::DDS::DataReader_ptr reader)
{
  R_var pt_dr
    = R::_narrow(reader);
  if (CORBA::is_nil (pt_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) _narrow failed.\n")));
      throw TestException() ;
    }

  const ::CORBA::Long max_read_samples = 100;
  Tseq samples(max_read_samples);
  ::DDS::SampleInfoSeq infos(max_read_samples);

  // wait for data to become available
  // so we know to start reading
  if (!Reader::wait_for_data(subscriber, 10))
    ACE_ERROR_RETURN((LM_ERROR,
      "ERROR: waited too long for the first sample\n"),
      -2);

  int num_reads = 0;
  int zero_reads = 0;
  int samples_recvd = 0;
  DDS::ReturnCode_t status;
  // initialize to zero.
  ::DDS::SampleRejectedStatus rejected;
  ::DDS::SampleLostStatus lost;
  if ((pt_dr->get_sample_rejected_status (rejected) != ::DDS::RETCODE_OK)
    || (pt_dr->get_sample_lost_status (lost) != ::DDS::RETCODE_OK))
  {
    ACE_ERROR((LM_ERROR,"ERROR: Failed to get sample reject or lost status.\n"));
    ACE_OS::exit (7);
  }

  while ( !stats->all_packets_received () )
    {

      status = pt_dr->read (
        samples,
        infos,
        max_read_samples,
        ::DDS::NOT_READ_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);
      //TBD do the right thing here.


      if (status == ::DDS::RETCODE_OK)
        {
          num_reads++;
          //if (samples.length () < 5)
          //  ACE_DEBUG((LM_DEBUG,"got just %d samples\n", samples.length ()));
          stats->samples_received(samples.length ());
          samples_recvd += samples.length ();
        }
      else if (status == ::DDS::RETCODE_NO_DATA)
        {
          zero_reads++;

          //ACE_DEBUG((LM_DEBUG,"got RETCODE_NO_DATA\n"));
          if (pt_dr->get_sample_rejected_status (rejected) != ::DDS::RETCODE_OK)
            {
              ACE_ERROR((LM_ERROR,
                ACE_TEXT ("ERROR: Failed to get sample rejected status.\n")));
              ACE_OS::exit (7);
            }

          if (rejected.total_count_change > 0)
            {
              //ACE_DEBUG((LM_DEBUG,"rejected %d samples\n", rejected.total_count_change));
              stats->samples_received(rejected.total_count_change);
            }

          if (pt_dr->get_sample_lost_status (lost) != ::DDS::RETCODE_OK)
            {
              ACE_ERROR((LM_ERROR,
                ACE_TEXT ("ERROR: Failed to get sample lost status.\n")));
              ACE_OS::exit (7);
            }

          if (lost.total_count_change > 0)
            {
              //ACE_DEBUG((LM_DEBUG,"lost %d samples\n", lost.total_count_change));
              stats->samples_received(lost.total_count_change);
            }

          // give the transport thread a chance to receive more and get the lock
          ACE_OS::thr_yield ();

          if (0) //zero_reads % 10000 == 0)
            {
              ACE_DEBUG((LM_DEBUG,"passed %d zero reads\n", zero_reads));
              ACE_DEBUG((LM_DEBUG,"samples = %d reads = %d zero_reads =%d samples per read = %d\n",
                stats->packet_count_, num_reads, zero_reads,
                stats->packet_count_ /num_reads));

              ACE_ERROR((LM_ERROR,"ERROR: samples rejected = %d lost =%d !!!!!!\n",
                rejected.total_count, lost.total_count));
            }

          if (zero_reads > 500000)
            {
              ACE_ERROR((LM_ERROR, "Too many zero_reads!!!!\n"));
              ACE_DEBUG((LM_DEBUG,"samples = %d reads = %d zero_reads =%d samples per read = %d\n",
                stats->packet_count_, num_reads, zero_reads,
                stats->packet_count_ /num_reads));

              ACE_ERROR((LM_ERROR,"ERROR: samples rejected = %d lost =%d read =%d total = %d !!!!!!\n",
                rejected.total_count,
                lost.total_count, samples_recvd,
                rejected.total_count + lost.total_count + samples_recvd));
              ACE_OS::sleep(2);
              ACE_OS::exit (7);
            }
        }
      else
       {
          num_reads++;
          zero_reads++;
          ACE_OS::printf (" read  data: Error: %d\n", status) ;
       }
    }

  ACE_DEBUG((LM_DEBUG,"samples = %d reads = %d zero_reads =%d samples per read = %d\n",
    stats->packet_count_, num_reads, zero_reads,
    stats->packet_count_ /num_reads));

  if (rejected.total_count > 0 || lost.total_count > 0)
    ACE_ERROR((LM_ERROR,"ERROR: samples rejected = %d lost =%d !!!!!!\n",
      rejected.total_count, lost.total_count));

  return ::DDS::RETCODE_OK;
}

Reader::Reader(::DDS::Subscriber_ptr subscriber,
               ::DDS::DataReader_ptr reader,
               int num_publishers,
               int num_samples,
               int data_size)
: subscriber_ (::DDS::Subscriber::_duplicate(subscriber)),
  reader_ (::DDS::DataReader::_duplicate(reader)),
  num_publishers_(num_publishers),
  num_samples_ (num_samples),
  data_size_ (data_size),
  num_floats_per_sample_ (data_size),
  finished_sending_ (false)
{
}

void
Reader::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Reader::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Reader::start, %p.\n"),
                ACE_TEXT("activate")));
    throw TestException ();
  }
}

void
Reader::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Reader::end \n")));
  wait ();
}


int
Reader::svc ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T Reader::svc begins \n")));

  int svc_status = 0;

  stats_.init(num_publishers_, num_samples_, data_size_, num_floats_per_sample_*4 + 8);
  // The above last parameter needs to be adjusted for the header

  DDS::ReturnCode_t status;

  try
  {

    switch ( num_floats_per_sample_ )
    {

    case 128:
      {
        status = read < ::Xyz::Pt128Seq,
                       ::Xyz::Pt128DataReader,
                       ::Xyz::Pt128DataReader_var,
                       ::Xyz::Pt128DataReader_ptr>
                         (
                          &stats_,
                          subscriber_.in (),
                          reader_.in ());
        if (::DDS::RETCODE_OK != status)
          return status;
      }
      break;

    case 512:
      {
        status = read < ::Xyz::Pt512Seq,
                       ::Xyz::Pt512DataReader,
                       ::Xyz::Pt512DataReader_var,
                       ::Xyz::Pt512DataReader_ptr>
                         (
                          &stats_,
                          subscriber_.in (),
                          reader_.in ());
        if (::DDS::RETCODE_OK != status)
          return status;
      }
      break;

    case 2048:
      {
        status = read < ::Xyz::Pt2048Seq,
                       ::Xyz::Pt2048DataReader,
                       ::Xyz::Pt2048DataReader_var,
                       ::Xyz::Pt2048DataReader_ptr>
                         (
                          &stats_,
                          subscriber_.in (),
                          reader_.in ());
        if (::DDS::RETCODE_OK != status)
          return status;
      }
      break;

    case 8192:
      {
        status = read < ::Xyz::Pt8192Seq,
                       ::Xyz::Pt8192DataReader,
                       ::Xyz::Pt8192DataReader_var,
                       ::Xyz::Pt8192DataReader_ptr>
                         (
                          &stats_,
                          subscriber_.in (),
                          reader_.in ());
        if (::DDS::RETCODE_OK != status)
          return status;
      }
      break;

    default:
      ACE_DEBUG((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
      return 1;
      break;
    };

  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
    return 1;
  }

  finished_sending_ = true;

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT(" Reader::svc finished.\n")));

  stats_.dump();

  return svc_status;
}

int
Reader::wait_for_data (::DDS::Subscriber_ptr sub,
                       int timeout_sec)
{
  const int factor = 1000;
  ACE_Time_Value small_time(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;
  ACE_Time_Value next_log = ACE_OS::gettimeofday () + ACE_Time_Value(5,0);

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.inout (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small_time);
      if (ACE_OS::gettimeofday () >= next_log)
        {
          ACE_DEBUG((LM_DEBUG,"%T Still waiting for data\n"));
          next_log = ACE_OS::gettimeofday () + ACE_Time_Value(5,0);
        }
    }
  return 0;
}


bool
Reader::is_finished () const
{
  return finished_sending_;
}

