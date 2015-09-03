#include "TestDriver.h"
#include "TcpPublisher.h"
#include "TestException.h"
#include <ace/Arg_Shifter.h>
#include <string>
#include "ace/Sched_Params.h"

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
    ACE_DEBUG ((LM_DEBUG, "WARNING: Failed to sched_params.\n"));
  }

#if (defined (MCL_CURRENT) && defined(MCL_FUTURE) && !defined(__ANDROID__))
  if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
    ACE_DEBUG ((LM_DEBUG, "WARNING:  Could not lock memory - Run with root access.\n"));
  }
#endif
}


TestDriver::TestDriver()
  : num_packets_(0),
    data_size_(0),
    packet_(0)
{
}


TestDriver::~TestDriver()
{
  delete packet_;
}


void
TestDriver::run(int& argc, ACE_TCHAR* argv[])
{
  set_rt();
  parse_args(argc, argv);
  init();
  run_i();
}


void
TestDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  // -n <num packets>
  // -d <data size>
  // -s <subscriber_host:subscriber_port>
  //
  // The -s option should be specified for each subscriber.
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;
  bool got_sub_port = false;

  while (arg_shifter.is_anything_left())
  {
    // The '-n' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n")))) {
      int tmp = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (tmp <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) value following -n cmd-line argument "
                   "must be greater than 0.\n"));
        throw TestException();
      }

      num_packets_ = tmp;
    }
    // The '-d' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-d")))) {
      int tmp = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (tmp <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) value following -d cmd-line argument "
                   "must be greater than 0.\n"));
        throw TestException();
      }

      data_size_ = tmp;
    }
    // The '-s' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-s")))) {
      std::string subscriber_spec = ACE_TEXT_ALWAYS_CHAR(current_arg);
      arg_shifter.consume_arg();
      subscriber_addr_ = parse_address(subscriber_spec);
      got_sub_port = true;
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-n num_packets "
                 "-d data_size -s subscriber_host:subscriber_port\n\n",
                 argv[0]));

      arg_shifter.consume_arg();
      throw TestException();
    }
    // Anything else we just skip
    else {
      arg_shifter.ignore_arg();
    }
  }

  // Make sure we got the required arguments:
  if (num_packets_ == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Num Packets (-n) not specified (required).\n"));
    throw TestException();
  }

  if (data_size_ == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Data Size (-d) not specified (required).\n"));
    throw TestException();
  }

  if (!got_sub_port) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Subscriber endpoint port (-s) not specified "
               "(required).\n"));
    throw TestException();
  }
}


void
TestDriver::init()
{
  if ((packet_ = TestData::PacketFactory::create_packet(data_size_)) == 0) {
    throw TestException();
  }

  packet_->set_publisher_id(1);

}


void
TestDriver::run_i()
{

  TcpPublisher publisher_(subscriber_addr_,
                          packet_->num_bytes());
  publisher_.connect();

  ACE_DEBUG((LM_DEBUG, "(%T) Publisher running.\n"));

  char* bytes        = packet_->as_bytes();
  unsigned num_bytes = packet_->num_bytes();
  unsigned total_packets = num_packets_ + 500;

  for (unsigned i = 0; i < total_packets; i++) {
    packet_->set_packet_id(i+1);
    publisher_.send_bytes(num_bytes, bytes);
  }

  publisher_.dump_stats();

  publisher_.disconnect();
}


ACE_INET_Addr
TestDriver::parse_address(const std::string& spec)
{
  std::string::size_type pos;

  if ((pos = spec.find(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad address (%C) - missing ':' char.\n",
               spec.c_str()));
    throw TestException();
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad address (%C) - "
               "':' char can't be first char.\n",
               spec.c_str()));
    throw TestException();
  }

  if (pos == (spec.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad address (%C) - "
               "':' char can't be last char.\n",
               spec.c_str()));
    throw TestException();
  }

  return ACE_INET_Addr(spec.c_str());
}
