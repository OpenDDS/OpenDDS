#include <ace/OS_NS_time.h>
#include <ace/High_Res_Timer.h>

#include "DummyTcp_export.h"

#include <string>
using namespace std;

class DummyTcp_Export PerformanceTest
{

  typedef struct
  {
    ACE_hrtime_t average;
    ACE_hrtime_t min;
    ACE_hrtime_t max;
    ACE_hrtime_t sum;
    ACE_hrtime_t sum2;
    int count;
  } stats_type;


public:
  static void start_test (const string& name, const string& start_loc);
  static void stop_test (const string& stop_loc);
  static ACE_hrtime_t transport_time ();
  static void report_stats ();
  static bool debug;
private:
  static ACE_High_Res_Timer timer_;
  static ACE_hrtime_t transport_time_;
  static bool started_;
  static stats_type result_;
  static std::string name_;
  static std::string start_location_;
  static std::string stop_location_;

  // helper functions
  static void add_stats (ACE_hrtime_t data);
  static void init_stats ();
  static double std_dev (const stats_type& result);

};
