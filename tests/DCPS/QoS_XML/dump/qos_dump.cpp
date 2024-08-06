
#include "dds/DCPS/QOS_XML_Handler/QOS_XML_Loader.h"
#include "dds/DdsDcpsC.h"

int ACE_TMAIN (int, ACE_TCHAR *[])
{
  int retval = 0;
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
                                ACE_TEXT("qos#TestProfile"),
                                ACE_TEXT("TopicName"));
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
          retcode_qos = xml_loader.get_datareader_qos (
                                dr_qos,
                                ACE_TEXT("qos#TestProfile"),
                                ACE_TEXT("TopicName"));
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_datareader_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          if (dr_qos.type_consistency.ignore_sequence_bounds != true)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos returned an invalid type type_consistency ignore_sequence_bounds.\n"));
            ++retval;
          }
          if (dr_qos.type_consistency.ignore_string_bounds != true)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos return an invalid type type_consistency ignore_string_bounds.\n"));
            ++retval;
          }
          if (dr_qos.type_consistency.ignore_member_names != true)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos return an invalid type type_consistency ignore_member_names.\n"));
            ++retval;
          }
          if (dr_qos.type_consistency.prevent_type_widening != true)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos return an invalid type type_consistency prevent_type_widening.\n"));
            ++retval;
          }
          if (dr_qos.type_consistency.force_type_validation != true)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos return an invalid type type_consistency force_type_validation.\n"));
            ++retval;
          }
          if (dr_qos.representation.value.length() != 2)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_datareader_qos return an invalid length %d for data_representation.\n",
                  dr_qos.representation.value.length()));
            ++retval;
          }

          ::DDS::TopicQos tp_qos;
          retcode_qos = xml_loader.get_topic_qos (
                                tp_qos,
                                ACE_TEXT("qos#TestProfile"),
                                ACE_TEXT("TopicName"));
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_topic_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          if (tp_qos.durability_service.history_kind != DDS::KEEP_LAST_HISTORY_QOS)
          {
            ACE_ERROR ((LM_ERROR, "PARSEXML - "
                  "get_topic_qos returned an invalid type durability_service history_kind.\n"));
            ++retval;
          }

          ::DDS::PublisherQos pub_qos;
          retcode_qos = xml_loader.get_publisher_qos (
                                pub_qos,
                                ACE_TEXT("qos#TestProfile"));
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_publisher_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::SubscriberQos sub_qos;
          retcode_qos = xml_loader.get_subscriber_qos (
                                sub_qos,
                                ACE_TEXT("qos#TestProfile"));
          if (retcode_qos != DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, "MAIN - "
                "get_subscriber_qos return an error. Retcode <%d>\n",
                retcode_qos));
              ++retval;
            }

          ::DDS::DomainParticipantQos dp_qos;
          retcode_qos = xml_loader.get_participant_qos (
                                dp_qos,
                                ACE_TEXT("qos#TestProfile"));
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
