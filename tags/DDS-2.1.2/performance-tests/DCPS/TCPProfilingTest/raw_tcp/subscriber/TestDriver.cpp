#include "TestDriver.h"
#include "TcpSubscriber.h"
#include "TestException.h"
#include "TestData.h"
#include <ace/Arg_Shifter.h>
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Reactor.h"

static bool verbose_ = false;

typedef ACE_Acceptor<TcpSubscriber, ACE_SOCK_ACCEPTOR> TcpSubscriber_Acceptor;


TestDriver::TestDriver()
  : num_publishers_(0),
    num_packets_(0),
    data_size_(0)
{
}


TestDriver::~TestDriver()
{
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
  // -p <num publishers>
  // -n <num packets>
  // -d <data size>
  // -s <subscriber_port>
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  bool got_port = false;

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

      num_publishers_ = tmp;
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
      addr_ = ACE_INET_Addr(current_arg);
      arg_shifter.consume_arg();
      got_port = true;
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p num_publishers -n num_packets "
                 "-d data_size -s subscriber_port\n",
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
  if (num_publishers_ == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Num Publishers (-p) not specified (required).\n"));
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

  if (!got_port) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Subscriber endpoint port (-s) not specified "
               "(required).\n"));
    throw TestException();
  }
}

void
TestDriver::init()
{
  // Create a temporary data packet just to get the num bytes per packet value.
  TestData::PacketHolder* packet;

  if ((packet = TestData::PacketFactory::create_packet(data_size_)) == 0) {
    throw TestException();
  }

  unsigned num_bytes_per_packet = packet->num_bytes();

  delete packet;

  // Init the stats
  stats_.init(num_publishers_,num_packets_,data_size_,num_bytes_per_packet);
}


void
TestDriver::run_i()
{
  ACE_Reactor* reactor = new ACE_Reactor();

  unsigned block_size = 1000000;
  unsigned num_blocks = 6;
  if ( num_blocks < num_publishers_)
  {
    num_blocks = num_publishers_ + 1;
  }

  TcpSubscriber::TestAllocator allocator(num_blocks,block_size);

  // Supply the stats object, the reactor, the buffer, and the buffer size
  // to the TcpSubscriber *class*.
  TcpSubscriber::initSubscriber(&stats_, reactor, &allocator, block_size);

  TcpSubscriber_Acceptor acceptor;

  if (acceptor.open(addr_, reactor) == -1) {
    ACE_ERROR((LM_ERROR,
               "%p\n",
               ACE_TEXT("open")));
    throw TestException();
  }

  ACE_DEBUG((LM_DEBUG, "(%T) Subscriber running.\n"));

  while (!stats_.all_packets_received()) {
    reactor->handle_events();
  }

  // Close the acceptor so that no more clients will be taken in.
  acceptor.close();

  // Free up the memory allocated for the reactor.
  delete reactor;

  stats_.dump();

  if (verbose_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Subscriber has completed.\n"));
  }
}
