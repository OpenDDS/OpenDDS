#include "DummyTcp_pch.h"
#include "PerformanceTest.h"
#include <ace/Log_Msg.h>
#include <iostream>
#include <cmath>

bool PerformanceTest::debug =false;
PerformanceTest::PTestMap PerformanceTest::pmap_;

using namespace std;

PerformanceTest::PerformanceTest (const string& test_name,
                                  const string& start_loc)
  : transport_time_(0),
    started_(false),
    test_name_(test_name),
    start_location_(start_loc)
{
    init_stats();
}

void
PerformanceTest::start_test (const string& name, const string& start_loc)
{
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::start_test\n"));

  // finding or creating a performance test object
 PerformanceTest* pt = find_or_create_testobj (name, start_loc);
  pt->begin_ptest();
}

void
PerformanceTest::stop_test (const string& name,
                            const string& stop_loc)
{
  PTestMap::iterator iter = pmap_.find(name);
  PerformanceTest* pt = 0;
  if (iter != pmap_.end())
    pt = iter->second;

  if(pt !=0)
  {
    if(pt->started())
    {
      pt->end_ptest(stop_loc);

      if(debug)
        ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::stop_test\n"));
    }
  }
  else
  {
    if(debug)
      ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::stop_test, cannot find the test object!\n"));
  }
}

PerformanceTest*
PerformanceTest::find_or_create_testobj(const string& test_name,
                                        const string& start_loc)
{

  PTestMap::iterator iter = pmap_.find(test_name);
  PerformanceTest* pt = 0;
  if (iter != pmap_.end())
    pt = iter->second;
  else
  {
    if(debug)
      ACE_DEBUG((LM_DEBUG,"(%P|%t), %s, Creating a new PerformanceTest object\n", test_name.c_str()));
    pt = new PerformanceTest(test_name,start_loc);
    pmap_.insert(make_pair(test_name,pt));
  }

  return pt;
}

void
PerformanceTest::cleanup (const string& test_name)
{
  PTestMap::iterator iter = pmap_.find(test_name);
  PerformanceTest* pt = 0;
  if (iter != pmap_.end())
    pt = iter->second;

  delete pt;

  pmap_.erase(iter);
}

void
PerformanceTest::calculate_transport_time ()
{
  timer_.elapsed_time (transport_time_);
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest::transport_time = %Q\n", transport_time_));
}

void
PerformanceTest::begin_ptest()
{
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest, start timer, count=%d\n", result_.count));
  timer_.start();
  started_ = true;

}
void
PerformanceTest::end_ptest(const string& stop_loc)
{
  if(debug)
    ACE_DEBUG((LM_DEBUG,"%T (%P|%t)PerformanceTest, stop timer\n"));
  timer_.stop();
  started_ = false;
  stop_location_ = stop_loc;
  calculate_transport_time();
  add_stats(transport_time_);
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
PerformanceTest::report_stats (const string& test_name)
{
  PTestMap::iterator iter = pmap_.find(test_name);
  PerformanceTest* pt = 0;
  if (iter != pmap_.end())
    pt = iter->second;

  pt->show_stats();
}

PerformanceTest::stats_type
PerformanceTest::stats ()
{
  return result_;
}

bool
PerformanceTest::started ()
{
  return started_;
}

void PerformanceTest::show_stats()
{
  time_t clock = time (NULL);
  std::cout << "\n# Transport performance measurements (in us) \n";
  std::cout << "Test Name: " << test_name_  << std::endl;
  std::cout << "Test Start Location: " << start_location_ << std::endl;
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
