#include <dds/DCPS/QOS_XML_Handler/XML_String_Intf.h>
#include <dds/DCPS/debug.h>

#include <dds/DdsDcpsC.h>

#include <fstream>
#include <sstream>
#include <string_view>

int parseXML(
  const char* fileName,
  std::string_view profileName,
  std::string_view topicName,
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
                      "TestProfile",
                      "TopicName",
                      xml1);
    if (retval > 0)
      return retval;
  }
  // Test second file
  // File name and profile name in respective file
  // topic name in respective profile name
  {
    retval += parseXML("append2test.xml",
                      "TestProfile2",
                      "TopicName",
                      xml2);
    if (retval > 0)
      return retval;
  }
  if (!xml1.getProfile("TestProfile"))
  {
    ACE_ERROR ((LM_ERROR, "MAIN - Could not find TestProfile in test.xml\n"));
    ++retval;
  }

  if (!xml2.getProfile("TestProfile2"))
  {
    ACE_ERROR ((LM_ERROR, "MAIN - Could not find TestProfile2 in append2test.xml\n"));
    ++retval;
  }

  if (xml2.get().qos_profile().size() != 5)
  {
    ACE_ERROR ((LM_ERROR, "MAIN - append2test.xml should contain five QoS profiles\n"));
    ++retval;
  }
  return retval;
}
