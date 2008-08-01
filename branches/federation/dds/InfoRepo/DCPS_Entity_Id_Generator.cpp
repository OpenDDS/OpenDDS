#include "DcpsInfo_pch.h"
#include "DCPS_Entity_Id_Generator.h"
#include "ace/Log_Msg.h"


static long lastParticipant = 0;
static long lastTopic       = 0;
static long lastSubPub      = 0;

DCPS_Entity_Id_Generator::DCPS_Entity_Id_Generator ()
{
}

DCPS_Entity_Id_Generator::~DCPS_Entity_Id_Generator (void)
{
}



OpenDDS::DCPS::RepoId
DCPS_Entity_Id_Generator::get_next_part_id(
  long federation
)
{
  if( ++lastParticipant == 0) {
    // We have rolled over and there can now exist objects with
    // the same key.
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_Entity_Key_Generator::get_next_part_id: ")
      ACE_TEXT("Exceeded Maximum number of keys!")
      ACE_TEXT("  Next key will be a duplicate!\n")
    ));

    // @TODO: We can do this to stop processing on wrap.
    // return OpenDDS::DCPS::GUID_UNKNOWN;
  }

  // Load Vendor Id, federation and participant values.
  OpenDDS::DCPS::GuidConverter converter( federation, lastParticipant);

  // Extract the GUID.
  OpenDDS::DCPS::RepoId value = converter;

  // Load the entityKey and entityKind values.
  value.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  return value;
}

OpenDDS::DCPS::RepoId
DCPS_Entity_Id_Generator::get_next_topic_id(
  long federation,
  const OpenDDS::DCPS::RepoId& pid
)
{
  if( (++lastTopic & ~OpenDDS::DCPS::GuidConverter::KeyMask) != 0) {
    // We have rolled over and there can now exist objects with
    // the same key.
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_Entity_Key_Generator::get_next_part_id: ")
      ACE_TEXT("Exceeded Maximum number of keys!")
      ACE_TEXT("  Next key will be a duplicate!\n")
    ));

    // @TODO: We can do this to stop processing on wrap.
    // return OpenDDS::DCPS::GUID_UNKNOWN;
  }

  long participant = OpenDDS::DCPS::GuidConverter(
                       const_cast<OpenDDS::DCPS::GUID_t*>(&pid)
                     ).participantId();
  OpenDDS::DCPS::GuidConverter converter( federation, participant);

  converter.key()[0] = (lastTopic >> 16) & 0xff;
  converter.key()[1] = (lastTopic >>  8) & 0xff;
  converter.key()[2] =  lastTopic        & 0xff;
  converter.kind()   = OpenDDS::DCPS::ENTITYKIND_OPENDDS_TOPIC;
  return converter;
}

OpenDDS::DCPS::RepoId
DCPS_Entity_Id_Generator::get_next_sub_pub_id(
  long federation,
  const OpenDDS::DCPS::RepoId& pid
)
{
  if( (++lastSubPub & ~OpenDDS::DCPS::GuidConverter::KeyMask) != 0) {
    // We have rolled over and there can now exist objects with
    // the same key.
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_Entity_Key_Generator::get_next_part_id: ")
      ACE_TEXT("Exceeded Maximum number of keys!")
      ACE_TEXT("  Next key will be a duplicate!\n")
    ));

    // @TODO: We can do this to stop processing on wrap.
    // return OpenDDS::DCPS::GUID_UNKNOWN;
  }

  long participant = OpenDDS::DCPS::GuidConverter(
                       const_cast<OpenDDS::DCPS::GUID_t*>(&pid)
                     ).participantId();
  OpenDDS::DCPS::GuidConverter converter( federation, participant);

  converter.key()[0] = (lastSubPub >> 16) & 0xff;
  converter.key()[1] = (lastSubPub >>  8) & 0xff;
  converter.key()[2] =  lastSubPub        & 0xff;
  converter.kind()   = OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY;
  return converter;
}

