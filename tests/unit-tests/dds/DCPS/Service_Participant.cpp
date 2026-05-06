#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticDiscovery.h>
#include <dds/DCPS/TimeTypes.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

namespace {

class TestDiscovery : public StaticDiscovery {
public:
  TestDiscovery(const RepoKey& key, bool active, int active_after_calls = 0)
    : StaticDiscovery(key)
    , active_(active)
    , active_after_calls_(active_after_calls)
    , active_calls_(0)
  {}

  bool active()
  {
    ++active_calls_;
    if (active_after_calls_ && active_calls_ >= active_after_calls_) {
      return true;
    }
    return active_;
  }

  int active_calls() const
  {
    return active_calls_;
  }

private:
  bool active_;
  int active_after_calls_;
  int active_calls_;
};

}

TEST(dds_DCPS_Service_Participant, type_object_encoding) {
  Service_Participant sp;

  EXPECT_EQ(sp.type_object_encoding(), Service_Participant::Encoding_Normal);
  sp.type_object_encoding(Service_Participant::Encoding_WriteOldFormat);
  EXPECT_EQ(sp.type_object_encoding(), Service_Participant::Encoding_WriteOldFormat);
  sp.type_object_encoding("ReadOldFormat");
  EXPECT_EQ(sp.type_object_encoding(), Service_Participant::Encoding_ReadOldFormat);
}

TEST(dds_DCPS_Service_Participant, event_dispatcher_thread_count)
{
  Service_Participant sp;

  EXPECT_EQ(sp.event_dispatcher_thread_count(), COMMON_DCPS_EVENT_DISPATCHER_THREADS_default);

  sp.event_dispatcher_thread_count(4);
  EXPECT_EQ(sp.event_dispatcher_thread_count(), 4u);

  sp.event_dispatcher_thread_count(0);
  EXPECT_EQ(sp.event_dispatcher_thread_count(), COMMON_DCPS_EVENT_DISPATCHER_THREADS_default);

  sp.config_store()->set_uint32(COMMON_DCPS_EVENT_DISPATCHER_THREADS, 0);
  EXPECT_EQ(sp.event_dispatcher_thread_count(), COMMON_DCPS_EVENT_DISPATCHER_THREADS_default);
}

TEST(dds_DCPS_Service_Participant, repository_lost_remaps_to_next_active_discovery)
{
  Service_Participant sp;

  const Discovery::RepoKey lost("repo-1");
  const Discovery::RepoKey unavailable("repo-2");
  const Discovery::RepoKey replacement("repo-3");
  const DDS::DomainId_t domain = 42;

  sp.add_discovery(make_rch<TestDiscovery>(lost, false));
  sp.add_discovery(make_rch<TestDiscovery>(unavailable, false));
  sp.add_discovery(make_rch<TestDiscovery>(replacement, true));
  sp.set_repo_domain(domain, lost, false);

  sp.repository_lost(lost);

  EXPECT_EQ(replacement, sp.domain_to_repo(domain));
}

TEST(dds_DCPS_Service_Participant, repository_lost_backs_off_when_lost_key_is_missing)
{
  Service_Participant sp;

  const Discovery::RepoKey lost("missing-repo");
  const Discovery::RepoKey replacement("repo-2");

  RcHandle<TestDiscovery> replacement_discovery = make_rch<TestDiscovery>(replacement, false, 2);
  sp.add_discovery(replacement_discovery);
  sp.federation_recovery_duration(5);
  sp.federation_initial_backoff_seconds(1);
  sp.federation_backoff_multiplier(1);

  const MonotonicTimePoint start = MonotonicTimePoint::now();
  sp.repository_lost(lost);
  const TimeDuration elapsed = MonotonicTimePoint::now() - start;

  EXPECT_EQ(2, replacement_discovery->active_calls());
  EXPECT_GE(elapsed, TimeDuration(1));
}
