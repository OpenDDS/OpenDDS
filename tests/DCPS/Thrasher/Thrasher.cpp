#include "Thrasher.h"
#include "PublisherService.h"
#include "Subscriber.h"

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>

#include <stdexcept>

Thrasher::Thrasher(int& argc, ACE_TCHAR** argv)
  : dpf_()
  , n_pub_threads_(DefaultPubThreads)
  , samples_per_thread_(DefaultSamplesPerThread)
  , expected_samples_(DefaultExpectedSamples)
  , durable_(false)
{
  try {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf_) {
      throw std::runtime_error("TheParticipantFactoryWithArgs failed.");
    }
    parse_args(argc, argv);
    ACE_DEBUG((LM_INFO, "(%P|%t) -> Thrasher started\n"));
  } catch (...) {
    cleanup();
    throw;
  }
}

int Thrasher::run()
{
  int ret = -1;
  Subscriber sub(DomainID, n_pub_threads_, expected_samples_, durable_);
  PublisherService pub_svc(DomainID, samples_per_thread_, durable_);
  if (pub_svc.start(n_pub_threads_)) {
    ret = sub.wait_and_check_received();
    pub_svc.end();
  }
  return ret;
}

void Thrasher::parse_args(int& argc, ACE_TCHAR** argv)
{
  ACE_Arg_Shifter shifter(argc, argv);
  while (shifter.is_anything_left()) {
    const ACE_TCHAR* arg;
    if ((arg = shifter.get_the_parameter(ACE_TEXT("-t")))) {
      n_pub_threads_ = ACE_OS::atoi(arg);
      shifter.consume_arg();
    } else if ((arg = shifter.get_the_parameter(ACE_TEXT("-s")))) {
      samples_per_thread_ = ACE_OS::atoi(arg);
      shifter.consume_arg();
    } else if ((arg = shifter.get_the_parameter(ACE_TEXT("-n")))) {
      expected_samples_ = ACE_OS::atoi(arg);
      shifter.consume_arg();
    } else if (ACE_OS::strcmp(shifter.get_current(), ACE_TEXT("-d")) == 0) {
      durable_ = true;
      shifter.consume_arg();
    } else {
      shifter.ignore_arg();
    }
  }
}

void Thrasher::cleanup()
{
  if (dpf_) {
    ACE_DEBUG((LM_INFO, "(%P|%t) <- Thrasher shutdown\n"));
    TheServiceParticipant->shutdown();
    dpf_ = 0;
  }
}
