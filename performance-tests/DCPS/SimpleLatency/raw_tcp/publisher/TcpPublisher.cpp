#include "TcpPublisher.h"
#include "TestException.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"

#include <ace/streams.h>
#include <iostream>
#include <math.h>
#include "ace/os_include/netinet/os_tcp.h"
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
    cout << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (data))
              << endl;

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





TcpPublisher::TcpPublisher(const ACE_INET_Addr& sub_addr,
                           unsigned /*message_size*/)
  : subscriber_addr_(sub_addr)
  , buffer_(100000)
  , pkt_count_(0)
{
  init_stats (round_trip, "round_trip");
}


TcpPublisher::~TcpPublisher()
{
}


void
TcpPublisher::connect()
{
  ACE_SOCK_Connector connector;

  if (connector.connect(subscriber_, subscriber_addr_) == -1) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot connect to subscriber.\n"));
    throw TestException();
  } // if (connector.connect(subscriber_, subscriber_addr_) == -1)

#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#  if !defined (ACE_LACKS_SOCKET_BUFSIZ)
  int nodelay =1;
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;

  if (subscriber_.set_option (IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof (nodelay)) == -1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) Publisher failed to set TCP_NODELAY\n"));
  }

  if (subscriber_.set_option (SOL_SOCKET, SO_SNDBUF, (void *) &snd_size,
                              sizeof (snd_size)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher failed to set the send buffer size to %d errno %m\n",
               snd_size));
  }

  if (subscriber_.set_option (SOL_SOCKET, SO_RCVBUF, (void *) &rcv_size,
                               sizeof (int)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher failed to set the receive buffer size to %d errno %m \n",
               rcv_size));
  }
#  endif /* !ACE_LACKS_SOCKET_BUFSIZ */
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */

}


void
TcpPublisher::disconnect()
{
  if (subscriber_.close() == -1 ) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot disconnect from the subscriber.\n"));
  }
}


void
TcpPublisher::send_bytes(unsigned num_bytes, const char* bytes)
{
  ACE_High_Res_Timer timer;
  ACE_hrtime_t perPacket;

  timer.start();

  if (subscriber_.send_n(bytes, num_bytes) == -1) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot send bytes to subscriber.\n"));
    throw TestException();
  } // if (subscriber_.send_n(bytes, num_bytes) == -1)

  ssize_t result;

  if ((result = subscriber_.recv(buffer_.wr_ptr(), 4)) == 0) {
    // The publisher has disconnected - check if this was unexpected.
    ACE_ERROR((LM_ERROR, "(%P|%t) Subscriber disconnected.\n"));
    throw TestException();
  }
  else if (result < 0) {
    // Something bad happened
    ACE_ERROR((LM_ERROR, "(%P|%t) bad read\n"));
    throw TestException();
  }
  else if (result != 4) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Failed to read in the required %d bytes. Read %d bytes.\n",
               4, static_cast<int>(result)));
    throw TestException();
  }

  timer.stop();
  if (++pkt_count_ > 500)
  {
    timer.elapsed_time(perPacket);
    add_stats (round_trip, perPacket);
  }
}



void
TcpPublisher::dump_stats ()
{
  time_t clock = ACE_OS::time (NULL);
  cout << "# MY Pub Sub measurements (in us) \n";
  cout << "# Executed at:" << ACE_OS::ctime(&clock);
  cout << "#       Roundtrip time [us]\n";
  cout << "Count     mean      min      max   std_dev\n";
  cout << " "
            << round_trip.count
            << "        "
            << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.average))
            << "     "
            << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.min))
            << "      "
            << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (round_trip.max))
            << "      "
            << std_dev (round_trip)
            << endl;
}

