#include "ace/OS_NS_string.h"
#include "ace/OS_NS_sys_wait.h"
#include "ace/Thread.h"
#include "ace/Thread_Manager.h"
#include "ace/SOCK_Dgram.h"
#include "ace/Log_Msg.h"
#include "ace/Time_Value.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_time.h"
#include "ace/Get_Opt.h"
#include <numeric>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>

const int max_buffer_size = 32000;

ACE_INET_Addr server_addr;
int frequency = 100; // messages per second
int message_size = 100;
int test_duration = 120; // seconds
int server_port = 0;

struct DataSample {
  ACE_hrtime_t sending_time_;
  char padding_[max_buffer_size - sizeof(ACE_hrtime_t) ];
};


static void *
sender (void *arg)
{
  ACE_SOCK_Dgram& cli_dgram = *reinterpret_cast<ACE_SOCK_Dgram*>(arg);

  ACE_Time_Value sleep_time (0, 1000000/frequency);
  int counter = frequency * test_duration;
  DataSample data;

  while (counter--) {
    data.sending_time_ = ACE_OS::gethrtime();
    if (cli_dgram.send (&data, message_size, server_addr) == -1)
    {

      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("cli_dgram.send")));
      return 0;
    }
    ACE_OS::sleep(sleep_time);
  }

  cli_dgram.send (&data, 0, server_addr);

  return 0;
}


struct Statistics
{
  double mean, variance;
  double max, min;
};

Statistics get_stat(const std::vector<double>& data)
{
  Statistics result = {0.0, 0.0, 0, data[0]};

  for (std::size_t  i =0; i < data.size(); ++i) {
    result.mean = (data[i])/(i+1) + ( result.mean / (i+1) * i);
    result.max = std::max(result.max, data[i]);
    result.min = std::min(result.min, data[i]);
  }

  for (std::size_t i= 0; i < data.size(); ++i) {
    double delta = result.mean - data[i];
    result.variance = (delta*delta) /(i+1) + result.variance/(i+1) *i;
  }
  return result;
}


static void
recevier(ACE_SOCK_Dgram& cli_dgram)
{
  ACE_INET_Addr peer_addr;
  DataSample data;
  ACE_Time_Value timeout(2,0);
  ssize_t rcv_cnt = 1;

  std::vector<double> latencies;
  latencies.reserve(frequency * test_duration);

  while (rcv_cnt) {
    rcv_cnt = cli_dgram.recv (&data,
                              sizeof (data),
                              peer_addr,
                              0,
                              &timeout);
    if (rcv_cnt > 0) {
      latencies.push_back(1E-9* (ACE_OS::gethrtime()-data.sending_time_));
    }
    else if (rcv_cnt < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) %p\n"),
                 ACE_TEXT("cli_dgram.recv")));
      return;
    }
  }

  Statistics result = get_stat(latencies) ;


  std::cout << " --- full path statistical summary --\n"
            << "     samples: " << latencies.size() << "\n"
            << "        mean: " << result.mean << "\n"
            << "     mininum: " << result.min << "\n"
            << "     maximum: " << result.max << "\n"
            << "    variance: " << result.variance << "\n\n"
            << " --- full path statistical data --\n";


  std::copy(latencies.begin(), latencies.end(), std::ostream_iterator<double>(std::cout, "\n"));
  return;
}


int echo_server(int port)
{

  ACE_INET_Addr server_addr;
  server_addr.set (port);
  ACE_SOCK_Dgram server_dgram;

  if (server_dgram.open (server_addr) == -1)
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) %p\n"),
               ACE_TEXT("server_dgram.open")));
    return 1;
  }

  ACE_INET_Addr peer_addr;
  DataSample data;
  ACE_Time_Value timeout(120,0);
  ssize_t rcv_cnt = 1;

  while (rcv_cnt) {
    rcv_cnt = server_dgram.recv (&data,
                              sizeof (data),
                              peer_addr,
                              0,
                              &timeout);
    if (rcv_cnt >= 0) {
      if (server_dgram.send(&data,
                            rcv_cnt,
                            peer_addr) < 0){
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) %p\n"),
                   ACE_TEXT("server_dgram.send")));
        return 1;
      }
    }
    else if (rcv_cnt < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) %p\n"),
                 ACE_TEXT("server_dgram.recv")));
      return 1;
    }
  }
  return 0;
}

int parse_args(int argc, ACE_TCHAR ** argv)
{
  int c;

  ACE_Get_Opt getopt(argc, argv, "c:d:f:m:s:");
  bool ok = true;

  while (ok && (c = getopt()) != -1) {
    switch (c) {
      case 'c':
        ok = (server_addr.set(getopt.opt_arg()) == 0);
        break;
      case 'd':
        test_duration = ACE_OS::atoi(getopt.opt_arg());
        ok = (test_duration != 0);
        break;
      case 'f':
        frequency = ACE_OS::atoi(getopt.opt_arg());
        ok = (frequency != 0);
        break;
      case 'm':
        message_size = ACE_OS::atoi(getopt.opt_arg());
        ok = (message_size != 0);
        break;
      case 's':
        server_port = ACE_OS::atoi(getopt.opt_arg());
        ok = (server_port != 0);
        break;
    }
  }

  // either server_port or server_addr has to be specified, but not both
  ok &= ( (!server_port) != (!server_addr.get_port_number()) );

  if (!ok) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("usage: %s [-c server_address [-d test_duration_in_sec] [-f frequency] [-m message_size] | -s server_port] \n"),
      argv[0]));
    return 1;
  }

  return !ok ;
}


int ACE_TMAIN (int argc, ACE_TCHAR ** argv)
{

  if (parse_args(argc, argv) == 0) {

    if (server_port) {
      echo_server(server_port);
    }
    else {

      ACE_SOCK_Dgram cli_dgram;

      if (cli_dgram.open (ACE_Addr::sap_any, server_addr.get_type ()) == -1)
        {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) %p\n"),
                     ACE_TEXT("cli_dgram.open")));
        }


        if (ACE_Thread_Manager::instance ()->spawn
            (ACE_THR_FUNC (sender),
             (void *) &cli_dgram,
             THR_NEW_LWP | THR_DETACHED) == -1)
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) %p\n"),
                             ACE_TEXT("thread create failed")),
                             1);

      recevier(cli_dgram);
      cli_dgram.close();
    }
    return 0;
  }

  return 1;
}
