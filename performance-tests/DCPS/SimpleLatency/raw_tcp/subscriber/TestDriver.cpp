#include "TestDriver.h"
#include "TestException.h"
#include "TestData.h"
#include <ace/Arg_Shifter.h>
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/os_include/netinet/os_tcp.h"
#include "ace/Sched_Params.h"

static bool verbose_ = false;

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
    num_bytes_per_packet_(0)
{
}


TestDriver::~TestDriver()
{
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
  // -s <subscriber_port>
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
      sub_addr_ = ACE_INET_Addr(current_arg);
      arg_shifter.consume_arg();
      got_sub_port = true;
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-n num_packets "
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
  // Create a temporary data packet just to get the num bytes per packet value.
  TestData::PacketHolder* packet;

  if ((packet = TestData::PacketFactory::create_packet(data_size_)) == 0) {
    throw TestException();
  }

  num_bytes_per_packet_ = packet->num_bytes();

  delete packet;
}


void
TestDriver::run_i()
{

  ACE_Message_Block buffer (1000000);

  ACE_SOCK_Acceptor acceptor;

  if (acceptor.open(sub_addr_) == -1) {
    ACE_ERROR((LM_ERROR,
               "%p\n",
               ACE_TEXT("open")));
    throw TestException();
  }

  ACE_SOCK_Stream peer;

  if (acceptor.accept(peer) == -1) {
    ACE_ERROR((LM_ERROR,
               "%p\n",
               ACE_TEXT("accept")));
    throw TestException();
  }

#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#  if !defined (ACE_LACKS_SOCKET_BUFSIZ)
  // set connection options
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int nodelay =1;

  if (peer.set_option (IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof (nodelay)) == -1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) Subscriber failed to set TCP_NODELAY\n"));
  }

  if (peer.set_option (SOL_SOCKET, SO_SNDBUF, (void *) &snd_size, sizeof (snd_size)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpSubscriber failed to set the send buffer size to %d errno %m\n",
               snd_size));
  }

  if (peer.set_option (SOL_SOCKET, SO_RCVBUF, (void *) &rcv_size, sizeof (int)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpSubscriber failed to set the receive buffer size to %d errno %m \n",
               rcv_size));
  }
#  endif /* !ACE_LACKS_SOCKET_BUFSIZ */
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */


  ACE_DEBUG((LM_DEBUG, "(%T) Subscriber running.\n"));

  unsigned total_packets = num_packets_ + 500;

  ssize_t result;
  for (unsigned pkt_cnt = 0; pkt_cnt < total_packets; ++pkt_cnt)
  {

    if ((result = peer.recv(buffer.wr_ptr(), num_bytes_per_packet_)) == 0) {
      // The publisher has disconnected - check if this was unexpected.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Publisher disconnected at packet %d.\n",
                 pkt_cnt));
      throw TestException();
    }
    else if (result < 0) {
      // Something bad happened
      ACE_ERROR((LM_ERROR, "(%P|%t) bad read\n"));
      throw TestException();
    }
    else if ((unsigned) result != num_bytes_per_packet_) {
      // Something bad happened
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) read %d bytes but expected %d\n",
                 static_cast<int>(result), num_bytes_per_packet_));
      throw TestException();
    }

    // only send 4 back
    result = 4;
    peer.send_n(buffer.wr_ptr(), static_cast<size_t>(result));
  }

  // Close the acceptor so that no more clients will be taken in.
  acceptor.close();

 if (verbose_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Subscriber has completed.\n"));
  }
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

