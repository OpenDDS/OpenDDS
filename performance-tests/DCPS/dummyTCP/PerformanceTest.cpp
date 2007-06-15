#include "PerformanceTest.h"
#include <ace/Log_Msg.h>
#include <iostream>
#include <cmath>

ACE_High_Res_Timer PerformanceTest::timer_;
PerformanceTest::stats_type PerformanceTest::result_;
ACE_hrtime_t PerformanceTest::transport_time_ = 0;
bool PerformanceTest::started_ = false;
bool PerformanceTest::debug =false;

std::string PerformanceTest::name_;
std::string PerformanceTest::start_location_;
std::string PerformanceTest::stop_location_;

void
PerformanceTest::start_test (const string& name, const string& start_loc)
{
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::start_test\n"));

  if (result_.count == 0)
  {
    if(debug)
      ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::start_test, count is 0, init_stats_\n"));

    init_stats();
  }

  name_ = name;
  start_location_ = start_loc;
  timer_.reset ();
  timer_.start ();
  started_ = true;
}

void
PerformanceTest::stop_test (const string& stop_loc)
{
  if(started_)
  {
    timer_.stop ();
    started_ = false;

    stop_location_ = stop_loc;
    transport_time ();
    if(debug)
    {
      ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::stop_test\n"));
      ACE_DEBUG((LM_DEBUG,"%T (%P|%t)Time spent in transport framework is %Q microseconds\n", transport_time_/(ACE_hrtime_t)1000));
    }

    add_stats(transport_time_);
  }
}

ACE_hrtime_t
PerformanceTest::transport_time ()
{
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::transport_time\n"));
  timer_.elapsed_time (transport_time_);
  return transport_time_;
}

void
PerformanceTest::add_stats (ACE_hrtime_t data)
{
    data = data / (ACE_hrtime_t) 1000;
    result_.average = (result_.count * result_.average + data)/(result_.count + 1);
    result_.min     = (result_.count == 0 || data < result_.min) ? data : result_.min;
    result_.max     = (result_.count == 0 || data > result_.max) ? data : result_.max;
    result_.sum = result_.sum + data;
    result_.sum2 = result_.sum2 + data * data;
    result_.count++;
}

void
PerformanceTest::init_stats ()
{
    result_.count    = 0;
    result_.average  = ACE_hrtime_t(0.0);
    result_.min      = ACE_hrtime_t(0.0);
    result_.max      = ACE_hrtime_t(0.0);
    result_.sum      = ACE_hrtime_t(0.0);
    result_.sum2     = ACE_hrtime_t(0.0);
}

double
PerformanceTest::std_dev (const stats_type& result)
{
  if (result.count >=2)
  {
    return std::sqrt ((static_cast<double>(result.count) * ACE_UINT64_DBLCAST_ADAPTER (result.sum2) -
                  ACE_UINT64_DBLCAST_ADAPTER (result.sum) * ACE_UINT64_DBLCAST_ADAPTER (result.sum)) /
                (static_cast<double>(result.count) * static_cast<double>(result.count - 1)));
  }
  return 0.0;
}

void
PerformanceTest::report_stats ()
{
  time_t clock = time (NULL);
  std::cout << "\n# Transport performance measurements (in us) \n";
  std::cout << "Test Name: " << name_  << std::endl;
  std::cout << "Test Start Location: " << start_location_  << std::endl;
  std::cout << "Test Stop  Location: " << stop_location_  << std::endl;
  std::cout << "# Executed at:" <<  ctime(&clock);
  std::cout << "# Transport framework traversal time [us]\n";
  std::cout << "Count     mean      min      max   std_dev\n";
  std::cout << " "
    << result_.count
    << "        "
    << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (result_.average))
    << "     "
    << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (result_.min))
    << "      "
    << static_cast<double> (ACE_UINT64_DBLCAST_ADAPTER (result_.max))
    << "      "
    << std_dev (result_)
    << std::endl;
}
