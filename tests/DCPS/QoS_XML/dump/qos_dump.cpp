
#include "dds/DCPS/QOS_XML_Handler/QOS_XML_Loader.h"
#include "dds/DdsDcpsC.h"

int ACE_TMAIN (int, ACE_TCHAR *[])
{
  try
    {
      OpenDDS::DCPS::QOS_XML_Loader xml_loader;
      DDS::ReturnCode_t const retcode = xml_loader.init (ACE_TEXT ("qos#TestProfile"));
      if (retcode == DDS::RETCODE_OK)
        {
          DDS::ReturnCode_t retcode_qos;
          ::DDS::DataWriterQos dw_qos;
          retcode_qos = xml_loader.get_datawriter_qos (
                                dw_qos,
                                "qos#TestProfile",
                                "TopicName");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datawriter_qos return an error. Retcode <%d>",
                retcode_qos));
            }

          ::DDS::DataReaderQos dr_qos;
          retcode_qos = xml_loader.get_datareader_qos (
                                dr_qos,
                                "qos#TestProfile",
                                "TopicName");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datareader_qos return an error. Retcode <%d>",
                retcode_qos));
            }

          ::DDS::TopicQos tp_qos;
          retcode_qos = xml_loader.get_topic_qos (
                                tp_qos,
                                "qos#TestProfile",
                                "TopicName");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_topic_qos return an error. Retcode <%d>",
                retcode_qos));
            }

          ::DDS::PublisherQos pub_qos;
          retcode_qos = xml_loader.get_publisher_qos (
                                pub_qos,
                                "qos#TestProfile");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_publisher_qos return an error. Retcode <%d>",
                retcode_qos));
            }

          ::DDS::SubscriberQos sub_qos;
          retcode_qos = xml_loader.get_subscriber_qos (
                                sub_qos,
                                "qos#TestProfile");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_subscriber_qos return an error. Retcode <%d>",
                retcode_qos));
            }

          ::DDS::DomainParticipantQos dp_qos;
          retcode_qos = xml_loader.get_participant_qos (
                                dp_qos,
                                "qos#TestProfile");
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_participant_qos return an error. Retcode <%d>",
                retcode_qos));
            }
        }
      else
        ACE_ERROR ((LM_ERROR, "MAIN - Init return an error. Retcode <%d>",
          retcode));
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

  return 0;
}
