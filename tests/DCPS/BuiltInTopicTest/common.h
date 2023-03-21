#ifndef COMMON_H
#define COMMON_H

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/StaticIncludes.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>

const char* topic_name = "Movie Discussion List";
const char* topic_type_name = "Messenger";

char PART_USER_DATA[] = "Initial DomainParticipant UserData";
char DW_USER_DATA[] = "Initial DataWriter UserData";
char DR_USER_DATA[] = "Initial DataReader UserData";
char TOPIC_DATA[] = "Initial Topic TopicData";
char GROUP_DATA[] = "Initial GroupData";
char UPDATED_PART_USER_DATA[] = "Updated DomainParticipant UserData";
char UPDATED_DW_USER_DATA[] = "Updated DataWriter UserData";
char UPDATED_DR_USER_DATA[] = "Updated DataReader UserData";
char UPDATED_TOPIC_DATA[] = "Updated Topic TopicData";
char UPDATED_GROUP_DATA[] = "Updated GroupData";

ACE_TString synch_dir;
ACE_TCHAR mon1_fname[] = ACE_TEXT("monitor1_done");
ACE_TCHAR mon2_fname[] = ACE_TEXT("monitor2_done");

int num_messages = 10;

#endif /* COMMON_H */
