#include <ace/OS_NS_time.h>
#include <ace/High_Res_Timer.h>

#include "DummyTcp_export.h"

#include <string>
#include <map>

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
  static void start_test (const std::string& name,
                          const std::string& start_loc);
  static void stop_test (const std::string& name,
                         const std::string& stop_loc);
  static void report_stats (const std::string& test_name);
  static double std_dev (const stats_type& result);
  static bool debug;
  static void cleanup (const std::string& test_name);
private:
  ACE_High_Res_Timer timer_;
  ACE_hrtime_t transport_time_;
  bool started_;
  stats_type result_;
  std::string test_name_;
  std::string start_location_;
  std::string stop_location_;

  // CTOR
  PerformanceTest(const std::string& test_name,
                  const std::string& start_loc);

  // helper functions
  void add_stats (ACE_hrtime_t data);
  void init_stats ();
  void calculate_transport_time ();
  void begin_ptest ();
  void end_ptest (const std::string& stop_loc);
  stats_type stats ();
  bool started ();
  void show_stats();

  typedef std::map<std::string,PerformanceTest*> PTestMap;
  static PTestMap pmap_;
  static PerformanceTest* find_or_create_testobj (const std::string& test_name,
                                          const std::string& start_loc);

};
