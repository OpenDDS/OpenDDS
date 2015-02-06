#include "TestDriver.h"
#include "TcpPublisher.h"
#include "TestException.h"
#include <ace/Arg_Shifter.h>
#include <string>


TestDriver::TestDriver()
  : publisher_id_(0),
    num_packets_(0),
    data_size_(0),
    packet_(0)
{
}


TestDriver::~TestDriver()
{
  delete packet_;

  PublisherList::iterator itr;
  PublisherList::iterator endItr = publishers_.end();

  for (itr = publishers_.begin(); itr != endItr; ++itr) {
    TcpPublisher* publisher = *itr;
    delete publisher;
  }

  publishers_.clear();
}


void
TestDriver::run(int& argc, ACE_TCHAR* argv[])
{
  parse_args(argc, argv);
  init();
  run_i();
}


void
TestDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  // -p <publisher id>
  // -n <num packets>
  // -d <data size>
  // -s <subscriber_host:subscriber_port>
  //
  // The -s option should be specified for each subscriber.
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-p")))) {
      int tmp = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (tmp <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) value following -p cmd-line argument "
                   "must be greater than 0.\n"));
        throw TestException();
      }

      publisher_id_ = tmp;
    }
    // The '-n' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n")))) {
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
    // A '-s' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-s")))) {
      std::string subscriber_spec = ACE_TEXT_ALWAYS_CHAR(current_arg);
      arg_shifter.consume_arg();
      subscriber_addrs_.push_back(parse_subscriber_address(subscriber_spec));
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p publisher_id -n num_packets "
                 "-d data_size -s subscriber_host:subscriber_port\n\n"
                 "The -s option may be specified more than once.\n",
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
  if (publisher_id_ == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher Id (-p) not specified (required).\n"));
    throw TestException();
  }

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

  if (subscriber_addrs_.size() == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Subscriber endpoint (-s) not specified "
               "(at least one is required).\n"));
    throw TestException();
  }
}


void
TestDriver::init()
{
  if ((packet_ = TestData::PacketFactory::create_packet(data_size_)) == 0) {
    throw TestException();
  }

  packet_->set_publisher_id(publisher_id_);

  AddrList::iterator itr;
  AddrList::iterator endItr = subscriber_addrs_.end();

  for (itr = subscriber_addrs_.begin(); itr != endItr; ++itr) {
    TcpPublisher* publisher = new TcpPublisher(*itr);
    publishers_.push_back(publisher);
    publisher->connect();
  }
}


void
TestDriver::run_i()
{
  ACE_DEBUG((LM_DEBUG, "(%T) Publisher running.\n"));

  char* bytes        = packet_->as_bytes();
  unsigned num_bytes = packet_->num_bytes();

  PublisherList::iterator itr;
  PublisherList::iterator endItr = publishers_.end();

  for (unsigned i = 0; i < num_packets_; i++) {
    packet_->set_packet_id(i+1);
    for (itr = publishers_.begin(); itr != endItr; ++itr) {
      (*itr)->send_bytes(num_bytes, bytes);
    }
  }

  for (itr = publishers_.begin(); itr != endItr; ++itr) {
    (*itr)->disconnect();
  }
}


ACE_INET_Addr
TestDriver::parse_subscriber_address(const std::string& spec)
{
  std::string::size_type pos;

  if ((pos = spec.find(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad subscriber address (%C) - missing ':' char.\n",
               spec.c_str()));
    throw TestException();
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad subscriber address (%C) - "
               "':' char can't be first char.\n",
               spec.c_str()));
    throw TestException();
  }

  if (pos == (spec.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad subscriber address (%C) - "
               "':' char can't be last char.\n",
               spec.c_str()));
    throw TestException();
  }

  return ACE_INET_Addr(spec.c_str());
}
