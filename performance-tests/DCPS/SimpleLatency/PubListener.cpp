// -*- C++ -*-
//
#include "PubListener.h"
#include "DDSPerfTestTypeSupportImpl.h"
#include "DDSPerfTestTypeSupportC.h"

#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>
#include <ace/OS_NS_time.h>

#include "tests/Utils/ExceptionStreams.h"

#include <time.h>
#include <math.h>
#include <string.h>

using namespace DDSPerfTest;
using namespace std;

typedef struct
{
    char name[20];
    ACE_hrtime_t average;
    ACE_hrtime_t min;
    ACE_hrtime_t max;
    ACE_hrtime_t sum;
    ACE_hrtime_t sum2;
    int count;
} stats_type;

//
// Static functions
//

static void
add_stats (
    stats_type& stats,
    ACE_hrtime_t data
    )
{
  data = data / (ACE_hrtime_t) 1000;
    std::cout << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (data))
              << std::endl;

    stats.average = (stats.count * stats.average + data)/(stats.count + 1);
    stats.min     = (stats.count == 0 || data < stats.min) ? data : stats.min;
    stats.max     = (stats.count == 0 || data > stats.max) ? data : stats.max;
    stats.sum = stats.sum + data;
    stats.sum2 = stats.sum2 + data * data;
    stats.count++;
}

static void
init_stats (
    stats_type& stats,
    const char *name)
{
    strncpy ((char *)stats.name, name, 19);
    stats.name[19] = '\0';
    stats.count    = 0;
    stats.average  = ACE_hrtime_t(0.0);
    stats.min      = ACE_hrtime_t(0.0);
    stats.max      = ACE_hrtime_t(0.0);
    stats.sum      = ACE_hrtime_t(0.0);
    stats.sum2     = ACE_hrtime_t(0.0);
}

static double
std_dev (stats_type& stats)
{
  if (stats.count >=2)
  {
    return sqrt ((static_cast<double>(stats.count) * ACE_UINT64_DBLCAST_ADAPTER (stats.sum2) -
                  ACE_UINT64_DBLCAST_ADAPTER (stats.sum) * ACE_UINT64_DBLCAST_ADAPTER (stats.sum)) /
                (static_cast<double>(stats.count) * static_cast<double>(stats.count - 1)));
  }
  return 0.0;
}




static stats_type round_trip;
//struct timeval round_pre_t;
//struct timeval round_post_t;


/* Defines number of warm-up samples which are used to avoid cold start issues */
#define TOTAL_PRIMER_SAMPLES      500
extern long total_samples;

AckDataReaderListenerImpl::AckDataReaderListenerImpl(CORBA::Long /*size*/)
  :writer_ (),
   reader_ (),
   dr_servant_ (),
   dw_servant_ (),
   handle_ (),
   sample_num_(1),
   done_ (0),
   use_zero_copy_(false)
{
  //
  // init timing statistics
  //
  init_stats (round_trip, "round_trip");

}

AckDataReaderListenerImpl::~AckDataReaderListenerImpl ()
{
}

void AckDataReaderListenerImpl::init(DDS::DataReader_ptr dr,
                                    DDS::DataWriter_ptr dw,
                                    bool use_zero_copy_read)
{
  this->writer_ = DDS::DataWriter::_duplicate (dw);
  this->reader_ = DDS::DataReader::_duplicate (dr);
  use_zero_copy_ = use_zero_copy_read;

  this->dr_servant_ =
    AckMessageDataReader::_narrow(this->reader_.in());

  this->dw_servant_ =
    PubMessageDataWriter::_narrow (this->writer_.in ());
  DDSPerfTest::PubMessage msg;
  this->handle_ = this->dw_servant_->register_instance(msg);
}


void AckDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr)
{
    CORBA::Long sequence_number;
    DDS::ReturnCode_t status;

    if (use_zero_copy_)
    {
      ::CORBA::Long max_read_samples = 1;
      AckMessageSeq messageZC(0, max_read_samples);
      DDS::SampleInfoSeq siZC(0, max_read_samples, 0);
      status = this->dr_servant_->take(messageZC,
                                       siZC,
                                       max_read_samples,
                                       ::DDS::NOT_READ_SAMPLE_STATE,
                                       ::DDS::ANY_VIEW_STATE,
                                       ::DDS::ANY_INSTANCE_STATE);

      timer_.stop();

      sequence_number = messageZC[0].seqnum;
      status = this->dr_servant_->return_loan(messageZC, siZC);
    }
    else
    {
      static DDSPerfTest::AckMessage message;
      static DDS::SampleInfo si;
      status = this->dr_servant_->take_next_sample(message, si) ;

      timer_.stop();

      sequence_number = message.seqnum;
    }

    if (status == DDS::RETCODE_OK) {
//      cout << "AckMessage: seqnum    = " << message.seqnum << endl;
    } else if (status == DDS::RETCODE_NO_DATA) {
      cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
    } else {
      cerr << "ERROR: read Message: Error: " <<  status << endl;
    }

    if (sequence_number != this->sample_num_)
    {
      fprintf(stderr,
              "ERROR - TAO_Pub: recieved seqnum %d on %d\n",
              sequence_number, this->sample_num_);

//      exit (1);
    }

    if (this->sample_num_ > TOTAL_PRIMER_SAMPLES)
    {
      ACE_hrtime_t round_trip_time;
      timer_.elapsed_time(round_trip_time);

      add_stats (round_trip, round_trip_time);
    }

    if (this->sample_num_ ==  total_samples + TOTAL_PRIMER_SAMPLES)
    {


      time_t clock = ACE_OS::time (NULL);
      std::cout << "# MY Pub Sub measurements (in us) \n";
      std::cout << "# Executed at:" << ACE_OS::ctime(&clock);
      std::cout << "#       Roundtrip time [us]\n";
      std::cout << "Count     mean      min      max   std_dev\n";
      std::cout << " "
                << round_trip.count
                << "        "
                << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.average))
                << "     "
                << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.min))
                << "      "
                << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.max))
                << "      "
                << std_dev (round_trip)
                << std::endl;


      DDSPerfTest::PubMessage msg;
      // send 0 to end the ping-pong operation
      msg.seqnum = 0;
      this->dw_servant_->write (msg, this->handle_);
      this->done_ = 1;
      return;
    }

    this->sample_num_++;

    DDSPerfTest::PubMessage msg;
    msg.seqnum = this->sample_num_;

    timer_.reset();
    timer_.start();
    this->dw_servant_->write (msg, this->handle_);

    return;
}

void AckDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
}

void AckDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
}

void AckDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
}

void AckDataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
}

void AckDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
}

void AckDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
}
