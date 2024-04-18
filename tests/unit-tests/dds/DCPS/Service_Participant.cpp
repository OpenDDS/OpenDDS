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
