#include "dds/DCPS/QOS_XML_Handler/QOS_XML_Loader.h"
#include "dds/DCPS/QOS_XML_Handler/XML_File_Intf.h"
#include "ace/Tokenizer_T.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  QOS_XML_Loader::QOS_XML_Loader(void)
  {
  }

  QOS_XML_Loader::~QOS_XML_Loader(void)
  {
  }


  ACE_TString
  QOS_XML_Loader::get_xml_file_name(const ACE_TCHAR* qos_profile)
  {
    if (qos_profile)
      {
        ACE_TCHAR* buf = ACE_OS::strdup(qos_profile);
        ACE_Tokenizer_T<ACE_TCHAR> tok(buf);
        tok.delimiter_replace('#', 0);
        const ACE_TCHAR * file_name = tok.next();

        if (file_name == 0)
          {
            ACE_OS::free(buf);
            if (DCPS_debug_level > 5)
              {
                ACE_ERROR((LM_ERROR,
                           "get_xml_file_name <%s> - "
                           "Error: malformed qos_profile. Expected format: "
                           "<xml_file_base_name>#<profile_name>\n",
                           qos_profile));
              }
            return ACE_TString();
          }

        ACE_TString ret(file_name);
        ret += ACE_TEXT(".xml");
        ACE_OS::free(buf);
        return ret;
      }

    return ACE_TString();
  }

  ACE_TString
  QOS_XML_Loader::get_profile_name(const ACE_TCHAR* qos_profile)
  {
    if (qos_profile)
      {
        ACE_TCHAR* buf = ACE_OS::strdup(qos_profile);
        ACE_Tokenizer_T<ACE_TCHAR> tok(buf);
        tok.delimiter_replace('#', 0);
        const ACE_TCHAR * lib_name = tok.next();
        const ACE_TCHAR * prof_name = tok.next();

        if (lib_name == 0 || prof_name == 0 || tok.next() != 0)
          {
            ACE_OS::free(buf);
            if (DCPS_debug_level > 5)
              {
                ACE_ERROR((LM_ERROR,
                             "get_profile_name <%s> - "
                             "Error: malformed qos_profile. Expected format: "
                             "<xml_file_base_name>#<profile_name>\n",
                             qos_profile));
              }
            return ACE_TString();
          }

        ACE_TString ret(prof_name);
        ACE_OS::free(buf);
        return ret;
      }

    return ACE_TString();
  }


  DDS::ReturnCode_t
  QOS_XML_Loader::init(const ACE_TCHAR* qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::init - ")
              ACE_TEXT("Passed an empty qos_profile, returning.\n")));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    ACE_TString filename = get_xml_file_name(qos_profile);

    if (filename.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::init - ")
              ACE_TEXT("Unable to extract a file name from <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    xml_file_.add_search_path(
      ACE_TEXT("DDS_ROOT"),
      ACE_TEXT("/docs/schema/"));

    DDS::ReturnCode_t const retcode =  xml_file_.init(filename.c_str());

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_datawriter_qos(
    ::DDS::DataWriterQos& dw_qos,
    const ACE_TCHAR * qos_profile,
    const ACE_TCHAR * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_datawriter_qos(dw_qos,
                                               profile_name.c_str(),
                                               topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_datareader_qos(
    DDS::DataReaderQos& dr_qos,
    const ACE_TCHAR * qos_profile,
    const ACE_TCHAR * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_datareader_qos(dr_qos,
                                               profile_name.c_str(),
                                               topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = ::DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_publisher_qos(
    DDS::PublisherQos& pub_qos,
    const ACE_TCHAR * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_publisher_qos(pub_qos, profile_name.c_str());
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_subscriber_qos(
    DDS::SubscriberQos& sub_qos,
    const ACE_TCHAR * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_subscriber_qos(sub_qos, profile_name.c_str());
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_topic_qos(
    DDS::TopicQos& topic_qos,
    const ACE_TCHAR * qos_profile,
    const ACE_TCHAR * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_topic_qos(topic_qos,
                                          profile_name.c_str(),
                                          topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_participant_qos(
    DDS::DomainParticipantQos& part_qos,
    const ACE_TCHAR * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT("No QOS profile provided. Can't do anything, ")
              ACE_TEXT("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    ACE_TString profile_name = get_profile_name(qos_profile);

    if (profile_name.empty())
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT("Error parsing profile string <%s>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = xml_file_.get_participant_qos(part_qos, profile_name.c_str());
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
