#include <dds/DCPS/Service_Participant.h>

#include <gtestWrapper.h>

using namespace OpenDDS::DCPS;

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
