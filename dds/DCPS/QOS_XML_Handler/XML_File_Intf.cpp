// $Id$

#include "XML_File_Intf.h"
#include "ace/XML_Utils/XML_Typedefs.h"
#include "ace/XML_Utils/XMLSchema/id_map.hpp"

#include "DataReaderQos_Handler.h"
#include "DataWriterQos_Handler.h"
#include "TopicQos_Handler.h"
#include "PublisherQos_Handler.h"
#include "SubscriberQos_Handler.h"
#include "ParticipantQos_Handler.h"

#include "dds/DCPS/debug.h"

namespace OpenDDS {
namespace DCPS {

  QOS_XML_File_Handler::QOS_XML_File_Handler (void)
  {
  }

  QOS_XML_File_Handler::~QOS_XML_File_Handler (void)
  {
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::init (const ACE_TCHAR * file)
  {
    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;
    try
      {
        if (!XML_Helper_type::XML_HELPER.is_initialized ())
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_File_Handler::init - ")
              ACE_TEXT ("Unable to initialize XML_Helper.\n")));
            return DDS::RETCODE_ERROR;
          }

        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_TRACE,
              ACE_TEXT ("QOS_XML_File_Handler::init - ")
              ACE_TEXT ("Constructing DOM\n")));
          }

        XERCES_CPP_NAMESPACE::DOMDocument *dom =
          XML_Helper_type::XML_HELPER.create_dom (file);

        if (dom == 0)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_File_Handler::init - ")
              ACE_TEXT ("Failed to open file %s\n"),
              file));
            return DDS::RETCODE_ERROR;
          }

        XERCES_CPP_NAMESPACE::DOMElement *profile_dom = dom->getDocumentElement ();

        if (DCPS_debug_level > 9)
          {
            ACE_DEBUG ((LM_TRACE,
              ACE_TEXT ("QOS_XML_File_Handler::init - ")
              ACE_TEXT ("DOMElement pointer: %u\n"), profile_dom));
          }

        ID_Map::TSS_ID_Map* TSS_ID_Map (ACE_Singleton<ID_Map::TSS_ID_Map, ACE_Null_Mutex>::instance());
        (*TSS_ID_Map)->reset ();

        this->profiles_ = dds::reader::dds (dom);
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::init - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML <%C> into IDL: %C\n"),
          file,
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::init - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML <%C> into IDL.\n"),
          file));
        retcode = DDS::RETCODE_ERROR;
      }
    return retcode;
  }

  ::dds::qosProfile *
  QOS_XML_File_Handler::get_profile (const char * profile_name)
  {
    for (::dds::qosProfile_seq::qos_profile_const_iterator it = this->profiles_.begin_qos_profile ();
        it != this->profiles_.end_qos_profile ();
        ++it)
      {
        if (ACE_OS::strcmp ((*it)->name ().c_str (), profile_name) == 0)
          {
            if (DCPS_debug_level > 7)
              {
                ACE_DEBUG ((LM_TRACE,
                  ACE_TEXT ("QOS_XML_File_Handler::get_profile - ")
                  ACE_TEXT ("Found profile <%C>\n"),
                  (*it)->name ().c_str ()));
              }
            return it->get();
          }
      }
    if (ACE_OS::strlen (profile_name) == 0)
      {
        ACE_ERROR ((LM_DEBUG,
          ACE_TEXT ("QOS_XML_File_Handler::get_profile - ")
          ACE_TEXT ("No profile specified\n")));
      }
    else
      {
        ACE_ERROR ((LM_TRACE,
          ACE_TEXT ("QOS_XML_File_Handler::get_profile - ")
          ACE_TEXT ("Did not find profile <%C>\n"),
          profile_name));
      }
    return 0;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_datawriter_qos (::DDS::DataWriterQos& dw_qos,
                                            const char * profile_name,
                                            const char * topic_name)
  {
    ACE_UNUSED_ARG (topic_name);

    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            DataWriterQos_Handler::get_datawriter_qos (dw_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_datawriter_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_datawriter_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_datareader_qos (::DDS::DataReaderQos& dr_qos,
                                            const char * profile_name,
                                            const char * topic_name)
  {
    ACE_UNUSED_ARG (topic_name);

    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            DataReaderQos_Handler::get_datareader_qos (dr_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_datareader_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_datareader_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_topic_qos (::DDS::TopicQos& tp_qos,
                                        const char * profile_name,
                                        const char * topic_name)
  {
    ACE_UNUSED_ARG (topic_name);

    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            TopicQos_Handler::get_topic_qos (tp_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_topic_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_topic_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_publisher_qos (::DDS::PublisherQos& pub_qos,
                                            const char * profile_name)
  {
    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            PublisherQos_Handler::get_publisher_qos (pub_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_publisher_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_publisher_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_subscriber_qos (::DDS::SubscriberQos& sub_qos,
                                            const char * profile_name)
  {
    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            SubscriberQos_Handler::get_subscriber_qos (sub_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_subscriber_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_subscriber_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_File_Handler::get_participant_qos (::DDS::DomainParticipantQos& sub_qos,
                                            const char * profile_name)
  {
    DDS::ReturnCode_t retcode = DDS::RETCODE_ERROR;
    try
      {
        ::dds::qosProfile * profile = this->get_profile (profile_name);
        if (profile != 0)
          {
            ParticipantQos_Handler::get_participant_qos (sub_qos, profile);
            retcode = ::DDS::RETCODE_OK;
          }
        else
          retcode = DDS::RETCODE_BAD_PARAMETER;
      }
    catch (const CORBA::Exception &ex)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_participant_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_File_Handler::get_participant_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  void
  QOS_XML_File_Handler::add_search_path (const ACE_TCHAR *environment,
                                          const ACE_TCHAR *relpath)
  {
    XML_Helper_type::XML_HELPER.get_resolver ().get_resolver ().add_path (environment, relpath);
  }

}
}
