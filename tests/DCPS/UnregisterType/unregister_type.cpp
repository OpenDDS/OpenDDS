#include "TestTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/StaticIncludes.h"

#include "tests/DCPS/common/TestSupport.h"


// const data declarations
const long  TEST_DOMAIN_NUMBER   = 17;
const char* TEST_TOPIC_NAME    = "test-topic-name";
const char* TEST_TYPE_NAME     = "test-type-name";
const char* BAD_TYPE_NAME = "bad-type-name";
const char* NULL_TYPE_NAME = 0;


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(TEST_DOMAIN_NUMBER,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (! CORBA::is_nil (dp.in ()));

      Ex::TestTypeSupport_var tts(new Ex::TestTypeSupportImpl());

      ::DDS::ReturnCode_t registerResult = tts->register_type(dp.in(), TEST_TYPE_NAME);
      TEST_CHECK(registerResult == ::DDS::RETCODE_OK);

      ::DDS::Topic_var topic = dp->create_topic(TEST_TOPIC_NAME, TEST_TYPE_NAME, TOPIC_QOS_DEFAULT, ::DDS::TopicListener::_nil(), ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK(!CORBA::is_nil(topic.in()));

      bool dcpsKeyCheck = tts->has_dcps_key();
      TEST_CHECK(dcpsKeyCheck);

      ::DDS::ReturnCode_t topicDeletionResult = dp->delete_topic(topic.in());
      TEST_CHECK(topicDeletionResult == ::DDS::RETCODE_OK);

      ::DDS::ReturnCode_t unregisterResult = tts->unregister_type(dp.in(), TEST_TYPE_NAME);
      TEST_CHECK(unregisterResult == DDS::RETCODE_OK);

      ::DDS::ReturnCode_t nullUnregisterResult = tts->unregister_type(dp.in(), NULL_TYPE_NAME);
      TEST_CHECK(nullUnregisterResult == ::DDS::RETCODE_BAD_PARAMETER);

      ::DDS::ReturnCode_t badUnregisterResult = tts->unregister_type(dp.in(), BAD_TYPE_NAME);
      TEST_CHECK(badUnregisterResult == ::DDS::RETCODE_ERROR);

      // Clean up the domain participant
      dp->delete_contained_entities();
      dpf->delete_participant(dp.in ());

      TheServiceParticipant->shutdown ();

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in unregister_type.cpp:");
      return 1;
    }

  return 0;
}
