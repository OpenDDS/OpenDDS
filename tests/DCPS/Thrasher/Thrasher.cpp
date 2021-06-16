#include "PublisherService.h"
#include "Subscriber.h"

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

class Thrasher
{
public:
  Thrasher(int& argc, ACE_TCHAR** argv);
  ~Thrasher() { cleanup(); }
  int run();
private:
  void parse_args(int& argc, ACE_TCHAR** argv);
  void cleanup();
  const long domain_id_;
  DDS::DomainParticipantFactory_var dpf_;
  int n_pub_threads_;
  std::size_t samples_per_thread_;
  std::size_t expected_samples_;
  bool durable_;
};

Thrasher::Thrasher(int& argc, ACE_TCHAR** argv)
  : domain_id_(42)
  , dpf_()
  , n_pub_threads_(1)
  , samples_per_thread_(1024)
  , expected_samples_(1024)
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
  Subscriber sub(domain_id_, n_pub_threads_, expected_samples_, durable_);
  PublisherService pub_svc(domain_id_, samples_per_thread_, durable_);
  pub_svc.start(n_pub_threads_);
  sub.wait(1, 2);
  sub.wait_received();
  pub_svc.end();
  sub.wait(0);
  return sub.check_result();
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
  if(dpf_){
    ACE_DEBUG((LM_INFO, "(%P|%t) <- Thrasher shutdown\n"));
    TheServiceParticipant->shutdown();
    dpf_ = 0;
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    Thrasher thrasher(argc, argv);
    return thrasher.run();
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: exception\n"));
  }
  return 1;
}
