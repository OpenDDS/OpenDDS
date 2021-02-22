#include <fstream>
#include <streambuf>
#include "dds/DCPS/QOS_XML_Handler/XML_MemBuf_Intf.h"
#include "dds/DdsDcpsC.h"

int ACE_TMAIN (int, ACE_TCHAR *[])
{
  int retval = 0;
  try
    {
		// File name and profile name in respective file
		const char * fileName = "qos.xml";
		const char * profileName = "TestProfile";
		const char * topicName = "TopicName";

		// read file and create string
		std::ifstream ifs(fileName);
		std::stringstream buffer;
		buffer << ifs.rdbuf();

		OpenDDS::DCPS::QOS_XML_MemBuf_Handler xml_membuf;
		// Add env variable and path to search schemas
		xml_membuf.add_search_path("DDS_ROOT","/docs/schema/");
		// initialize and parse XML
		DDS::ReturnCode_t const retcode = xml_membuf.init(buffer.str().c_str());

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
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datawriter_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }
          if (dw_qos.history.kind != ::DDS::KEEP_ALL_HISTORY_QOS)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datawriter_qos return an invalid history kind.\n"));
              ++retval;
            }
          if (dw_qos.history.depth != 5)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datawriter_qos return an invalid history depth.\n"));
              ++retval;
            }

          ::DDS::DataReaderQos dr_qos;
          retcode_qos = xml_membuf.get_datareader_qos (
                                dr_qos,
                                profileName,
                                topicName);
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datareader_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::TopicQos tp_qos;
          retcode_qos = xml_membuf.get_topic_qos (
                                tp_qos,
                                profileName,
                                topicName);
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_topic_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::PublisherQos pub_qos;
          retcode_qos = xml_membuf.get_publisher_qos (
                                pub_qos,
                                profileName);
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_publisher_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::SubscriberQos sub_qos;
          retcode_qos = xml_membuf.get_subscriber_qos (
                                sub_qos,
                                profileName);
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_subscriber_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::DomainParticipantQos dp_qos;
          retcode_qos = xml_membuf.get_participant_qos (
                                dp_qos,
                                profileName);
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_participant_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }
        }
      else
        {
          ACE_ERROR ((LM_ERROR, "MAIN - Init return an error. Retcode <%d>\n",
            retcode));
          ++retval;
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("QOS_Dump::main\n");
      return -1;
    }
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT ("Unexpected exception\n")));
      return 1;
    }

  return retval;
}
