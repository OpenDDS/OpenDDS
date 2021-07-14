#include <fstream>
#include <streambuf>
#include "dds/DCPS/QOS_XML_Handler/XML_String_Intf.h"
#include "dds/DdsDcpsC.h"
#include "dds/DCPS/debug.h"

int parseXML(const char* fileName,
            const ACE_TCHAR* profileName,
            const ACE_TCHAR* topicName,
            OpenDDS::DCPS::QOS_XML_String_Handler& xml_membuf)
{
  int retval = 0;

  try
  {

    // read file and create string
    std::ifstream ifs(fileName);
    std::stringstream buffer;
    buffer << ifs.rdbuf();

    // Add env variable and path to search schemas
    xml_membuf.add_search_path(ACE_TEXT("DDS_ROOT"),ACE_TEXT("/docs/schema/"));
    // initialize and parse XML
    DDS::ReturnCode_t const retcode = xml_membuf.init(ACE_TEXT_CHAR_TO_TCHAR (buffer.str().c_str()));

    if (retcode == DDS::RETCODE_OK)
    {
      DDS::ReturnCode_t retcode_qos;
      ::DDS::DataWriterQos dw_qos;
      retcode_qos = xml_membuf.get_datawriter_qos (
          dw_qos,
          profileName,
          topicName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_datawriter_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }

      if (dw_qos.history.kind != ::DDS::KEEP_ALL_HISTORY_QOS)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_datawriter_qos return an invalid history kind.\n",
              fileName));
        ++retval;
      }
      if (dw_qos.history.depth != 5)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_datawriter_qos return an invalid history depth.\n",
              fileName));
        ++retval;
      }

      ::DDS::DataReaderQos dr_qos;
      retcode_qos = xml_membuf.get_datareader_qos (
          dr_qos,
          profileName,
          topicName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_datareader_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }

      ::DDS::TopicQos tp_qos;
      retcode_qos = xml_membuf.get_topic_qos (
          tp_qos,
          profileName,
          topicName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_topic_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }

      ::DDS::PublisherQos pub_qos;
      retcode_qos = xml_membuf.get_publisher_qos (
          pub_qos,
          profileName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_publisher_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }

      ::DDS::SubscriberQos sub_qos;
      retcode_qos = xml_membuf.get_subscriber_qos (
          sub_qos,
          profileName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_subscriber_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }

      ::DDS::DomainParticipantQos dp_qos;
      retcode_qos = xml_membuf.get_participant_qos (
          dp_qos,
          profileName);
      if (retcode_qos != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR, "PARSEXML - "
              "%s : get_participant_qos return an error. Retcode <%d>\n",
              fileName, retcode_qos));
        ++retval;
      }
    }
    else
    {
      ACE_ERROR ((LM_ERROR, "Error processing file %s - Init return an error. Retcode <%d>\n",
            fileName, retcode));
      ++retval;
    }

  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("QOS_Dump::main\n");
    return 1;
  }
  catch (...)
  {
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("Unexpected exception\n")));
    return 1;
  }

  return retval;
};

int ACE_TMAIN (int, ACE_TCHAR *[])
{
  int retval = 0;
  OpenDDS::DCPS::QOS_XML_String_Handler xml1;
  OpenDDS::DCPS::QOS_XML_String_Handler xml2;

  // Test first file
  // File name and profile name in respective file
  // topic name in respective profile name
  {
    retval += parseXML("test.xml",
                      ACE_TEXT("TestProfile"),
                      ACE_TEXT("TopicName"),
                      xml1);
    if (retval > 0)
      return retval;
  }
  // Test second file
  // File name and profile name in respective file
  // topic name in respective profile name
  {
    retval += parseXML("append2test.xml",
                      ACE_TEXT("TestProfile2"),
                      ACE_TEXT("TopicName"),
                      xml2);
    if (retval > 0)
      return retval;
  }
  // Test xml_membuf operations
  // append a profile
  {
    const ACE_TCHAR* profileName = ACE_TEXT("TestA");
    dds::qosProfile profile = xml2.getProfile(profileName);
    DDS::ReturnCode_t retcode;
    retcode = xml1.addQoSProfile(profile);
    if (retcode != DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR, "MAIN - "
            "Cannot append profile name <%C>. Retcode <%d>\n",
            profileName, retcode));
      return ++retval;
    }
    // Check if it was inserted
    dds::qosProfile testA = xml2.getProfile(profileName);
    if (ACE_OS::strcmp(testA.name().c_str(),profileName) != 0)
    {
      ACE_ERROR ((LM_ERROR, "MAIN - "
            "Cannot get last profile name <%C> inserted. Retcode <%d>\n",
            profileName, retcode));
      return ++retval;
    }

    // Remove profile
    retcode = xml1.delQoSProfile(profileName);
    if (retcode != DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR, "MAIN - "
            "Cannot remove profile name <%C>. Retcode <%d>\n",
            profileName, retcode));
      return ++retval;
    }
  }
  // append existing profile name
  {
    const ACE_TCHAR* profileName = ACE_TEXT("TestProfile");
    dds::qosProfile profile = xml2.getProfile(profileName);
    DDS::ReturnCode_t retcode = xml1.addQoSProfile(profile);
    if (retcode == DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR, "MAIN - "
            "Unexpected insertion of profile name <%C>.\n",
            profileName));
      return ++retval;
    }
  }
  // append list of profiles
  {
    const dds::qosProfile_seq& profiles = xml2.get();
    DDS::ReturnCode_t retcode = xml1.addQoSProfileSeq(profiles);
    if (retcode != DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR, "MAIN - "
            "Cannot append profile list. RetCode %d\n",retcode));
      return ++retval;
    }
  }
  return retval;
}
