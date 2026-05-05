#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticDiscovery.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

namespace {

class TestDiscovery : public StaticDiscovery {
public:
  TestDiscovery(const RepoKey& key, bool active)
    : StaticDiscovery(key)
    , active_(active)
  {}

  bool active()
  {
    return active_;
  }

private:
  bool active_;
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
