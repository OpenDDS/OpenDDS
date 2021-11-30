#include "BenchC.h"
#include "PropertyStatBlock.h"

#include <json_conversion.h>

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
std::string report_file_name;

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

  if (cli_dgram.send (&data, 0, server_addr) == -1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("cli_dgram.send")));
  }

  return 0;
}

static void
receiver(ACE_SOCK_Dgram& cli_dgram)
{
  ACE_INET_Addr peer_addr;
  DataSample data;
  ACE_Time_Value timeout(2,0);
  ssize_t rcv_cnt = 1;

  Bench::WorkerReport report{};
  report.process_report.participants.length(1);
  report.process_report.participants[0].subscribers.length(1);
  report.process_report.participants[0].subscribers[0].datareaders.length(1);
  Builder::PropertySeq& properties = report.process_report.participants[0].subscribers[0].datareaders[0].properties;

  const size_t buffer_size = static_cast<size_t>(1.1 * frequency * test_duration);

  auto round_trip_latency_stat_block = std::make_shared<Bench::PropertyStatBlock>(properties, "round_trip_latency", buffer_size);

  while (rcv_cnt) {
    rcv_cnt = cli_dgram.recv (&data,
                              sizeof (data),
                              peer_addr,
                              0,
                              &timeout);
    if (rcv_cnt > 0) {
      round_trip_latency_stat_block->update(5E-10 * (ACE_OS::gethrtime() - data.sending_time_));
    }
    else if (rcv_cnt < 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) %p\n"),
                 ACE_TEXT("cli_dgram.recv")));
      return;
    }
  }

  round_trip_latency_stat_block->finalize();

  if (!report_file_name.empty()) {
    ofstream report_file(report_file_name);
    if (report_file.good()) {
      idl_2_json(report, report_file, 9u);
    }
  }

  return;
}

int echo_server(int port)
{

  ACE_INET_Addr server_addr;
  server_addr.set (port);
  ACE_SOCK_Dgram server_dgram;

  if (server_dgram.open (server_addr) == -1) {
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
                            peer_addr) < 0) {
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

  ACE_Get_Opt getopt(argc, argv, "c:d:f:m:s:r:");
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
      case 'r':
        report_file_name = ACE_TEXT_ALWAYS_CHAR(getopt.opt_arg());
        ok = (!report_file_name.empty());
        break;
    }
  }

  // either server_port or server_addr has to be specified, but not both
  ok &= ( (!server_port) != (!server_addr.get_port_number()) );

  if (!ok) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("usage: %s [-c server_address [-d test_duration_in_sec] [-f frequency] [-m message_size] [-r report_file] | -s server_port]\n"),
      argv[0]));
    return 1;
  }

  return !ok ;
}

int ACE_TMAIN (int argc, ACE_TCHAR ** argv)
{
  try {

    if (parse_args(argc, argv) == 0) {

      if (server_port) {
        echo_server(server_port);
      }
      else {

        ACE_SOCK_Dgram cli_dgram;

        if (cli_dgram.open (ACE_Addr::sap_any, server_addr.get_type ()) == -1) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) %p\n"),
                     ACE_TEXT("cli_dgram.open")));
        }

          if (ACE_Thread_Manager::instance ()->spawn
              (ACE_THR_FUNC (sender),
              (void *) &cli_dgram,
              THR_NEW_LWP | THR_DETACHED) == -1) {
            ACE_ERROR_RETURN ((LM_ERROR,
                               ACE_TEXT("(%P|%t) %p\n"),
                               ACE_TEXT("thread create failed")),
                               1);
          }

        receiver(cli_dgram);
        cli_dgram.close();
      }
      return 0;
    }
  } catch (...) {
    ACE_ERROR((LM_ERROR, "Unknown Exception Caught"));
  }

  return 1;
}
