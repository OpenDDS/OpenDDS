#include "dds/DCPS/QOS_XML_Handler/QOS_XML_Loader.h"
#include "dds/DCPS/QOS_XML_Handler/XML_File_Intf.h"
#include "ace/Tokenizer_T.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  QOS_XML_Loader::QOS_XML_Loader (void)
  {
  }

  QOS_XML_Loader::~QOS_XML_Loader (void)
  {
  }


  char*
  QOS_XML_Loader::get_xml_file_name (const char * qos_profile)
  {
    if (qos_profile)
      {
        char* buf = ACE_OS::strdup (qos_profile);
        ACE_Tokenizer_T<char> tok (buf);
        tok.delimiter_replace ('#', 0);
        const char * file_name = tok.next ();

        if (file_name == 0)
          {
            ACE_OS::free (buf);
            if (DCPS_debug_level > 5)
              {
                ACE_ERROR ((LM_ERROR,
                              "get_xml_file_name <%C> - "
                              "Error: malformed qos_profile. Expected format: "
                              "<xml_file_base_name>#<profile_name>\n",
                              qos_profile));
              }
            return 0;
          }

        char * ret = (char*)ACE_OS::malloc (ACE_OS::strlen (file_name) + 5);
        ret = ACE_OS::strcpy (ret, file_name);
        ret = ACE_OS::strcat (ret, ".xml");
        ACE_OS::free (buf);
        return ret;
      }

    return 0;
  }

  char*
  QOS_XML_Loader::get_profile_name (const char * qos_profile)
  {
    if (qos_profile)
      {
        char* buf = ACE_OS::strdup (qos_profile);
        ACE_Tokenizer_T<char> tok (buf);
        tok.delimiter_replace ('#', 0);
        const char * lib_name = tok.next ();
        const char * prof_name = tok.next ();

        if (lib_name == 0 || prof_name == 0 || tok.next () != 0)
          {
            ACE_OS::free (buf);
            if (DCPS_debug_level > 5)
              {
                ACE_ERROR ((LM_ERROR,
                              "get_profile_name <%C> - "
                              "Error: malformed qos_profile. Expected format: "
                              "<xml_file_base_name>#<profile_name>\n",
                              qos_profile));
              }
            return 0;
          }

        char * ret = ACE_OS::strdup (prof_name);
        ACE_OS::free (buf);
        return ret;
      }

    return 0;
  }


  DDS::ReturnCode_t
  QOS_XML_Loader::init (const char * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::init - ")
              ACE_TEXT ("Passed an empty qos_profile, returning.\n")));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    char *filename = this->get_xml_file_name (qos_profile);

    if (!filename)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::init - ")
              ACE_TEXT ("Unable to extract a file name from <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    this->xml_file_.add_search_path (
      ACE_TEXT("DDS_ROOT"),
      ACE_TEXT("/docs/schema/"));

    DDS::ReturnCode_t const retcode =  this->xml_file_.init (filename);

    ACE_OS::free (filename);

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_datawriter_qos (
    ::DDS::DataWriterQos& dw_qos,
    const char * qos_profile,
    const char * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_datawriter_qos (dw_qos,
                                                      profile_name,
                                                      topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_datawriter_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }
    ACE_OS::free (profile_name);
    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_datareader_qos (
    DDS::DataReaderQos& dr_qos,
    const char * qos_profile,
    const char * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_datareader_qos (dr_qos,
                                                      profile_name,
                                                      topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_datareader_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = ::DDS::RETCODE_ERROR;
      }
    ACE_OS::free (profile_name);

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_publisher_qos (
    DDS::PublisherQos& pub_qos,
    const char * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_publisher_qos (pub_qos, profile_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_publisher_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }
    ACE_OS::free (profile_name);

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_subscriber_qos (
    DDS::SubscriberQos& sub_qos,
    const char * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_subscriber_qos (sub_qos, profile_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_subscriber_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    ACE_OS::free (profile_name);

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_topic_qos (
    DDS::TopicQos& topic_qos,
    const char * qos_profile,
    const char * topic_name)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_topic_qos (topic_qos,
                                                 profile_name,
                                                 topic_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_topic_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    ACE_OS::free (profile_name);

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Loader::get_participant_qos (
    DDS::DomainParticipantQos& part_qos,
    const char * qos_profile)
  {
    if (!qos_profile)
      {
        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT ("No QOS profile provided. Can't do anything, ")
              ACE_TEXT ("returning\n")));
          }

        return DDS::RETCODE_OK;
      }

    char* profile_name = this->get_profile_name (qos_profile);

    if (!profile_name)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT ("Error parsing profile string <%C>, returning.\n"),
              qos_profile));
          }
        return ::DDS::RETCODE_BAD_PARAMETER;
      }

    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;

    try
      {
        retcode = this->xml_file_.get_participant_qos (part_qos, profile_name);
      }
    catch (...)
      {
        if (DCPS_debug_level > 5)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_Loader::get_participant_qos - ")
              ACE_TEXT ("Caught unexpected exception.\n")));
          }
        retcode = DDS::RETCODE_ERROR;
      }

    ACE_OS::free (profile_name);

    return retcode;
  }
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
