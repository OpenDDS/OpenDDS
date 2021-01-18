// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================

#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/WaitForSample.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/StaticIncludes.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>

using namespace std;

int delay_before_read_sec = 0;
unsigned int num_parts = 3;
unsigned int num_topics = 2;
unsigned int num_subs = 1;
unsigned int num_pubs = 1;
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

char* CUR_PART_USER_DATA = PART_USER_DATA;
char* CUR_DW_USER_DATA = DW_USER_DATA;
char* CUR_DR_USER_DATA = DR_USER_DATA;
char* CUR_TOPIC_DATA = TOPIC_DATA;
char* CUR_GROUP_DATA = GROUP_DATA;

unsigned int dps_with_user_data = 2;
ACE_TString synch_dir;
ACE_TCHAR synch_fname[] = ACE_TEXT("monitor1_done");

int
parse_args(int argc, ACE_TCHAR* argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("T:l:d:t:s:p:u"));
  int c;

  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'T':
      synch_dir = get_opts.opt_arg();
      break;
    case 'l':
      delay_before_read_sec = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 'd':
      num_parts = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 't':
      num_topics = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 's':
      num_subs = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 'p':
      num_pubs = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case 'u': // Check with reset qos data.
      CUR_PART_USER_DATA = UPDATED_PART_USER_DATA;
      CUR_DW_USER_DATA = UPDATED_DW_USER_DATA;
      CUR_DR_USER_DATA = UPDATED_DR_USER_DATA;
      CUR_TOPIC_DATA = UPDATED_TOPIC_DATA;
      CUR_GROUP_DATA = UPDATED_GROUP_DATA;
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
        "usage:  %s "
        "-l <delay_before_read_sec> "
        "-d <num of domain participant> "
        "-t <num of topics> "
        "-s <num of subs> "
        "-p <num of pubs> "
        "-u "
        "-T <Where to put monitor1_done>"
        "\n",
        argv[0]),
        -1);
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor main\n"));

    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;

    dpf = TheParticipantFactoryWithArgs(argc, argv);

    if (parse_args(argc, argv) == -1) {
      return -1;
    }

    if (CUR_PART_USER_DATA == UPDATED_PART_USER_DATA) {
      // wait for Monitor 1 done
      FILE* fp = ACE_OS::fopen((synch_dir + synch_fname).c_str(), ACE_TEXT("r"));
      int i = 0;
      while (fp == 0 && i < 15) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) waiting for monitor1 to finish...\n")));
        ACE_OS::sleep(1);
        ++i;
        fp = ACE_OS::fopen((synch_dir + synch_fname).c_str(), ACE_TEXT("r"));
      }

      if (fp != 0) ACE_OS::fclose(fp);
      // Debug begin
      else {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("Monitor2 -- Monitor1 probably hasn't finished yet...\n")));
      }
      // Debug end
    }

    participant = dpf->create_participant(111,
                                          PARTICIPANT_QOS_DEFAULT,
                                          DDS::DomainParticipantListener::_nil(),
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) monitor: create_participant failed.\n"));
      return 1 ;
    }

    OpenDDS::DCPS::Discovery_rch disc =
      TheServiceParticipant->get_discovery(participant->get_domain_id());

    OpenDDS::DCPS::DomainParticipantImpl* part_svt
      = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());

    if (!part_svt) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) monitor: ")
        ACE_TEXT("failed to obtain DomainParticipantImpl.\n")));
      return 1;
    }

    // Debug begin
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Monitor's participant ID: %C\n"),
               OpenDDS::DCPS::to_string(part_svt->get_id()).c_str()));
    // Debug end

    const bool ignoredEntitiesAreInBIT = !dynamic_cast<OpenDDS::RTPS::RtpsDiscovery*>(disc.in());
    const bool ownEntitiesAreInBIT = ignoredEntitiesAreInBIT;

    // give time for BIT datareader/datawriter fully association.
    ACE_OS::sleep(5);

    if (delay_before_read_sec > 0) {
      ACE_DEBUG((LM_DEBUG,"(%P|%t) monitor: SLEEPING BEFORE READING!\n"));
      ACE_OS::sleep(delay_before_read_sec);
    }

    ::DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();

    ::DDS::DataReader_var part_rdr =
        bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);

    ::DDS::ParticipantBuiltinTopicDataDataReader_var part_reader
        = ::DDS::ParticipantBuiltinTopicDataDataReader::_narrow(part_rdr.in());
    if (CORBA::is_nil(part_reader.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) monitor: failed to get BUILT_IN_PARTICIPANT_TOPIC datareader.\n"));
      return 1;
    }

    ::DDS::DataReader_var topic_rdr =
        bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC);
    ::DDS::TopicBuiltinTopicDataDataReader_var topic_reader =
        ::DDS::TopicBuiltinTopicDataDataReader::_narrow(topic_rdr.in());
    if (CORBA::is_nil(topic_reader.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) monitor: failed to get BUILT_IN_TOPIC_TOPIC datareader.\n"));
      return 1;
    }

    ::DDS::DataReader_var subscription_rdr =
        bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    ::DDS::SubscriptionBuiltinTopicDataDataReader_var sub_reader =
        ::DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(subscription_rdr.in());
    if (CORBA::is_nil(sub_reader.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) monitor: failed to get BUILT_IN_SUBSCRIPTION_TOPIC datareader.\n"));
      return 1;
    }

    ::DDS::DataReader_var publication_rdr =
        bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    ::DDS::PublicationBuiltinTopicDataDataReader_var pub_reader =
        ::DDS::PublicationBuiltinTopicDataDataReader::_narrow(publication_rdr.in());
    if (CORBA::is_nil(pub_reader.in())) {
      ACE_ERROR((LM_ERROR, "(%P|%t) monitor: failed to get BUILT_IN_PUBLICATION_TOPIC datareader.\n"));
      return 1;
    }

    // Debug ========= Check built-in participant topic data
    {
      ::DDS::InstanceHandleSeq handles;
      if (participant->get_discovered_participants(handles) != ::DDS::RETCODE_OK
          || handles.length() == 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) monitor: get_discovered_participants test failed.\n"));
        return 1;
      }

      CORBA::ULong len = handles.length();
      if (len != num_parts - 1) {
        // Debug begin
        for (unsigned i = 0; i < len; ++i) {
          DDS::ParticipantBuiltinTopicData data;
          participant->get_discovered_participant_data(data, handles[i]);
          OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::bit_key_to_repo_id(data.key);
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("Discovered participant id: %C\n"),
                     OpenDDS::DCPS::to_string(id).c_str()));
        }
        // Debug end

        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) ERROR: monitor: get_discovered_participants expected %d got %d.\n",
          num_parts - 1, len), 1);
      }

      for (CORBA::ULong i = 0; i < len; ++i) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) monitor: participant %d examining participant handle %d.\n"),
          participant->get_instance_handle(), handles[i]));
        ::DDS::ParticipantBuiltinTopicData data;
        participant->get_discovered_participant_data(data, handles[i]);

        OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::bit_key_to_repo_id(data.key);

        if (part_svt->id_to_handle(id) != handles[i]) {
          ACE_ERROR((LM_ERROR, "(%P|%t) monitor: get_discovered_participant_data test failed.\n"));
          return 1;
        }
      }

      if (participant->ignore_participant(handles[0]) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) monitor: ignore_participant failed.\n"));
        return 1;
      }

      handles.length(0);
      if (participant->get_discovered_participants(handles) != ::DDS::RETCODE_OK
          || handles.length() != num_parts - 2) {
        ACE_ERROR((LM_ERROR, ACE_TEXT ("(%P|%t) monitor: get_discovered_participant ")
                   ACE_TEXT ("skip ignored participant test failed.\n")));
        return 1;
      }

      // Debug: if the ignored participant is from the RTPS peer, reduce num_parts?
      // Else do nothing if the ignored participant is from the DCPSInfoRepo?
      if (!ignoredEntitiesAreInBIT) --num_parts;

      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: discover participants test PASSED.\n"));
    }

    ::DDS::ReturnCode_t ret;

    ::DDS::ParticipantBuiltinTopicDataSeq partdata;
    // SonDN: expected_part_count == 1?
    const CORBA::ULong expected_part_count = num_parts - !ownEntitiesAreInBIT;

    while (partdata.length() != expected_part_count) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: waiting for participant sample\n"));
      Utils::waitForSample(part_rdr);
      ::DDS::SampleInfoSeq pinfos(10);
      ::DDS::ParticipantBuiltinTopicDataSeq pdata(10);
      ret = part_reader->read(pdata,
                              pinfos,
                              10,
                              ::DDS::ANY_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ALIVE_INSTANCE_STATE);

      if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "(%P|%t) monitor:  failed to read BIT participant data.\n"),
                         1);
      }

      for (CORBA::ULong i = 0; i < pdata.length(); ++i) {
        const CORBA::ULong idx = partdata.length();
        partdata.length(idx + 1);
        partdata[idx] = pdata[i];
      }
    }

    CORBA::ULong cur_dps_with_user_data = 0;
    CORBA::ULong user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_PART_USER_DATA));

    for (CORBA::ULong i = 0; i < partdata.length(); ++i) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: Participant: key = %d, %x, %x \n",
        partdata[i].key.value[0], partdata[i].key.value[1], partdata[i].key.value[2]));

      CORBA::ULong cur_len = partdata[i].user_data.value.length();

      if (cur_len == user_data_len &&
          ACE_OS::strncmp(reinterpret_cast<char*>(partdata[i].user_data.value.get_buffer()),
                          CUR_PART_USER_DATA, user_data_len) == 0) {
        ++cur_dps_with_user_data;
      }
    }

    if (cur_dps_with_user_data == dps_with_user_data - !ignoredEntitiesAreInBIT) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DomainParticipant changeable qos test PASSED.\n"));
    } else {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor:  DomainParticipant changeable qos test FAILED.\n"), 1);
    }

    // Debug ========= Check built-in topic topic data
    {
      //      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: waiting for topic sample\n"));
      // Debug: temporarily comment out this line
      //      Utils::waitForSample(topic_rdr);
      // Debug begin
      //      ACE_DEBUG((LM_DEBUG, "monitor: finished waiting for topic sample\n"));
      // Debug end
      ::DDS::InstanceHandleSeq handles;
      if (participant->get_discovered_topics(handles) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) monitor: get_discovered_topics test failed.\n"));
        return 1;
      }

      CORBA::ULong len = handles.length();
      if (len != num_topics) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor: ERROR: get_discovered_topics expected %d got %d.\n",
          num_topics, len), 1);
      }
      // Debug begin
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("Discovered %d topics, expected %d topics\n"),
                 len, num_topics));
      // Debug end

      for (CORBA::ULong i = 0; i < len; ++i) {
        ::DDS::TopicBuiltinTopicData data;
        participant->get_discovered_topic_data(data, handles[i]);

        OpenDDS::DCPS::Discovery_rch disc =
          TheServiceParticipant->get_discovery(participant->get_domain_id());
        OpenDDS::DCPS::RepoId id = OpenDDS::DCPS::bit_key_to_repo_id(data.key);

        if (part_svt->id_to_handle(id) != handles[i]) {
          ACE_ERROR((LM_ERROR, "(%P|%t) monitor: get_discovered_topic_data test failed.\n"));
          return 1;
        }
      }

      if (len && participant->ignore_topic(handles[0]) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) monitor: ignore_topic failed.\n"));
        return 1;
      }

      handles.length(0);
      if (len && (participant->get_discovered_topics(handles) != ::DDS::RETCODE_OK
                  || handles.length() != num_topics - 1)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) monitor: get_discovered_topics "
                   "skip ignored topic test failed with len = %d, "
                   "handles.length() = %d, num_topics = %d\n",
                   len, handles.length(), num_topics));
        return 1;
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: discover topics test PASSED.\n"));
    }

    if (ignoredEntitiesAreInBIT) {
    ::DDS::SampleInfoSeq topicinfos(10);
    ::DDS::TopicBuiltinTopicDataSeq topicdata(10);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: waiting for topic sample\n"));
    Utils::waitForSample(topic_rdr);
    ret = topic_reader->read(topicdata,
                             topicinfos,
                             10,
                             ::DDS::ANY_SAMPLE_STATE,
                             ::DDS::ANY_VIEW_STATE,
                             ::DDS::ANY_INSTANCE_STATE);

    if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA) {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor:  failed to read BIT topic data.\n"), 1);
    }

    CORBA::ULong len = topicdata.length();

    if (len != num_topics) {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor: ERROR: read %d BIT topic data, expected %d topics.\n", len, num_topics),
        1);
    }
    // Debug begin
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Read %d BIT topics, expected %d topics\n"), len, num_topics));
    // Debug end

    CORBA::ULong num_topics_with_data = 0;
    for (CORBA::ULong i = 0; i < len; ++i) {
      if (ACE_OS::strcmp(topicdata[i].name.in(), topic_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor: ERROR: got topic name \"%C\", expected topic name \"%C\"\n",
          topicdata[i].name.in(), topic_name), 1);
      }
      // Debug begin
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%dth topic -- topic name: %C\n"), i, topicdata[i].name.in()));
      // Debug end

      if (ACE_OS::strcmp(topicdata[i].type_name.in(), topic_type_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor:  got topic type name \"%C\", expected topic type name \"%C\"\n",
          topicdata[i].type_name.in(), topic_type_name), 1);
      }
      // Debug begin
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%dth topic -- type name: %C\n"), i, topicdata[i].type_name.in()));
      // Debug end

      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: Topic: key = %d, %x, %x, name = %C, "
        "type_name=%C \n",
        topicdata[i].key.value[0], topicdata[i].key.value[1], topicdata[i].key.value[2],
        topicdata[i].name.in(), topicdata[i].type_name.in()));

      CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_TOPIC_DATA));

      if (topicdata[i].topic_data.value.length() == topic_data_len &&
          ACE_OS::strncmp(reinterpret_cast <char*>(topicdata[i].topic_data.value.get_buffer()),
                          CUR_TOPIC_DATA, topic_data_len) == 0) {
        ++num_topics_with_data;
      }
    }

    if (num_topics_with_data == num_topics) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: Topic changeable qos test PASSED. \n"));
    } else {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor:  Topic changeable qos test FAILED. \n"), 1);
    }
    }

    // Debug ========== Check built-in publication topic data
    ::DDS::SampleInfoSeq pubinfos(10);
    ::DDS::PublicationBuiltinTopicDataSeq pubdata(10);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: waiting for publication sample\n"));
    Utils::waitForSample(publication_rdr);
    ret = pub_reader->read(pubdata,
                           pubinfos,
                           10,
                           ::DDS::ANY_SAMPLE_STATE,
                           ::DDS::ANY_VIEW_STATE,
                           ::DDS::ALIVE_INSTANCE_STATE);

    if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA) {
      ACE_ERROR_RETURN ((LM_ERROR,
        "(%P|%t) monitor:  failed to read BIT publication data.\n"), 1);
    }

    CORBA::ULong len = pubdata.length();
    bool pubWasIgnored = false;
    if (len != num_pubs) {
      if (!ignoredEntitiesAreInBIT && len == num_pubs - 1) {
        pubWasIgnored = true;
        ACE_DEBUG((LM_INFO, "(%P|%t) monitor: pub assumed to be ignored\n"));
      } else {
        ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) monitor:  read %d BIT pub data,"
                          "expected %d pubs.\n", len, num_pubs), 1);
      }
    }

    CORBA::ULong num_dws_with_data = 0;
    for (CORBA::ULong i = 0; i < len; ++i) {
      if (ACE_OS::strcmp(pubdata[i].topic_name.in(), topic_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor:  got datawriter topic name \"%C\", expected topic name \"%C\"\n",
            pubdata[i].topic_name.in(), topic_name), 1);
      }
      if (ACE_OS::strcmp(pubdata[i].type_name.in(), topic_type_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor:  got datawriter topic type name \"%C\", expected topic type name \"%C\"\n",
          pubdata[i].type_name.in(), topic_type_name), 1);
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DataWriter: key = %d, %x, %x. \n",
                 pubdata[i].key.value[0], pubdata[i].key.value[1], pubdata[i].key.value[2]));

      //ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DW user data %C \n", pubdata[i].user_data.value.get_buffer()));
      //ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DW topic data %C \n", pubdata[i].topic_data.value.get_buffer()));
      //ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DW group data %C \n", pubdata[i].group_data.value.get_buffer()));

      CORBA::ULong user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_DW_USER_DATA));
      CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_TOPIC_DATA));
      CORBA::ULong group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_GROUP_DATA));

      if (pubdata[i].user_data.value.length() == user_data_len &&
          pubdata[i].topic_data.value.length() == topic_data_len &&
          pubdata[i].group_data.value.length() == group_data_len) {
        if (ACE_OS::strncmp(reinterpret_cast<char*>(pubdata[i].user_data.value.get_buffer()),
                            CUR_DW_USER_DATA, user_data_len) == 0 &&
            ACE_OS::strncmp(reinterpret_cast<char*>(pubdata[i].topic_data.value.get_buffer()),
                            CUR_TOPIC_DATA, topic_data_len) == 0 &&
            ACE_OS::strncmp(reinterpret_cast<char*>(pubdata[i].group_data.value.get_buffer()),
                            CUR_GROUP_DATA, group_data_len) == 0) {
          ++num_dws_with_data;
        }
      }
    }

    if (num_dws_with_data == len) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DataWriter changeable qos test PASSED. \n"));
    } else {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor: DataWriter changeable qos test FAILED. \n"), 1);
    }

    // Debug ========== Check built-in subscription topic data
    ::DDS::SampleInfoSeq subinfos(10);
    ::DDS::SubscriptionBuiltinTopicDataSeq subdata(10);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: waiting for subscription sample\n"));
    Utils::waitForSample(subscription_rdr);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: done waiting for subscription sample\n"));
    ret = sub_reader->read(subdata,
                           subinfos,
                           10,
                           ::DDS::ANY_SAMPLE_STATE,
                           ::DDS::ANY_VIEW_STATE,
                           ::DDS::ALIVE_INSTANCE_STATE);

    if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA) {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) monitor:  failed to read BIT subsciption data.\n"), 1);
    }

    len = subdata.length();

    if (len != num_subs) {
      if (!pubWasIgnored && !ignoredEntitiesAreInBIT && len == num_subs - 1) {
        ACE_DEBUG((LM_INFO, "(%P|%t) monitor: sub assumed to be ignored\n"));
      } else {
        ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) monitor:  read %d BIT sub data, "
                          "expected %d subs.\n", len, num_subs), 1);
      }
    }

    CORBA::ULong num_drs_with_data = 0;
    for (CORBA::ULong i = 0; i < len; ++i) {
      if (ACE_OS::strcmp(subdata[i].topic_name.in(), topic_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor:  got datareader topic name \"%C\", expected topic name \"%C\"\n",
          subdata[i].topic_name.in (), topic_name), 1);
      }
      if (ACE_OS::strcmp(subdata[i].type_name.in(), topic_type_name) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
          "(%P|%t) monitor:  got datareader topic type name \"%C\", expected topic type name \"%C\"\n",
          subdata[i].type_name.in (), topic_type_name), 1);
      }

      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DataReader: key = %d, %x, %x \n",
                 subdata[i].key.value[0], subdata[i].key.value[1], subdata[i].key.value[2]));

      //ACE_DEBUG((LM_DEBUG, "(%P|%t) DR user data %C \n", subdata[i].user_data.value.get_buffer()));
      //ACE_DEBUG((LM_DEBUG, "(%P|%t) DR topic data %C \n", subdata[i].topic_data.value.get_buffer()));
      //ACE_DEBUG((LM_DEBUG, "(%P|%t) DR group data %C \n", subdata[i].group_data.value.get_buffer()));

      CORBA::ULong user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_DR_USER_DATA));
      CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_TOPIC_DATA));
      CORBA::ULong group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(CUR_GROUP_DATA));

      if (subdata[i].user_data.value.length() == user_data_len &&
          subdata[i].topic_data.value.length() == topic_data_len &&
          subdata[i].group_data.value.length() == group_data_len) {
        if (ACE_OS::strncmp(reinterpret_cast<char*>(subdata[i].user_data.value.get_buffer()),
                            CUR_DR_USER_DATA, user_data_len) == 0 &&
            ACE_OS::strncmp(reinterpret_cast<char*>(subdata[i].topic_data.value.get_buffer()),
                            CUR_TOPIC_DATA, topic_data_len) == 0 &&
            ACE_OS::strncmp(reinterpret_cast<char*>(subdata[i].group_data.value.get_buffer()),
                            CUR_GROUP_DATA, group_data_len) == 0) {
          ++num_drs_with_data;
        }
      }
    }

    if (num_drs_with_data == len) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor: DataReader changeable qos test PASSED. \n"));
    } else {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) monitor: DataReader changeable qos test FAILED. \n"),
                       1);
    }

    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

    if (CUR_PART_USER_DATA == PART_USER_DATA) {
      // Create synch file.
      FILE* fp = ACE_OS::fopen((synch_dir + synch_fname).c_str(), ACE_TEXT("w"));
      if (fp != 0) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) monitor1 is done\n")));
        ACE_OS::fclose(fp);
      }
      // Debug begin
      else {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("Failed to create file monitor1_done!!!\n")));
      }
      // Debug end
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) monitor main done\n"));

  } catch (CORBA::Exception& e) {
    e._tao_print_exception("monitor: SUB: Exception caught in main ():", stderr);
    return 1;
  }

  return 0;
}
