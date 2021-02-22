
#include "XML_Intf.h"
#include "ace/XML_Utils/XML_Typedefs.h"
#include "ace/XML_Utils/XMLSchema/id_map.hpp"

#include "DataReaderQos_Handler.h"
#include "DataWriterQos_Handler.h"
#include "TopicQos_Handler.h"
#include "PublisherQos_Handler.h"
#include "SubscriberQos_Handler.h"
#include "ParticipantQos_Handler.h"

#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  QOS_XML_Handler::QOS_XML_Handler (void)
  {
  }

  QOS_XML_Handler::~QOS_XML_Handler (void)
  {
  }

  ::dds::qosProfile *
  QOS_XML_Handler::get_profile (const char * profile_name)
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
                  ACE_TEXT ("QOS_XML_Handler::get_profile - ")
                  ACE_TEXT ("Found profile <%C>\n"),
                  (*it)->name ().c_str ()));
              }
            return it->get();
          }
      }
    if (ACE_OS::strlen (profile_name) == 0)
      {
        ACE_ERROR ((LM_DEBUG,
          ACE_TEXT ("QOS_XML_Handler::get_profile - ")
          ACE_TEXT ("No profile specified\n")));
      }
    else
      {
        ACE_ERROR ((LM_TRACE,
          ACE_TEXT ("QOS_XML_Handler::get_profile - ")
          ACE_TEXT ("Did not find profile <%C>\n"),
          profile_name));
      }
    return 0;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_datawriter_qos (::DDS::DataWriterQos& dw_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_datawriter_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_datawriter_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_datareader_qos (::DDS::DataReaderQos& dr_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_datareader_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_datareader_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_topic_qos (::DDS::TopicQos& tp_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_topic_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_topic_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_publisher_qos (::DDS::PublisherQos& pub_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_publisher_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_publisher_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_subscriber_qos (::DDS::SubscriberQos& sub_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_subscriber_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_subscriber_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

  DDS::ReturnCode_t
  QOS_XML_Handler::get_participant_qos (::DDS::DomainParticipantQos& sub_qos,
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
          ACE_TEXT ("QOS_XML_Handler::get_participant_qos - ")
          ACE_TEXT ("Caught CORBA exception whilst parsing XML into IDL: %C\n"),
          ex._info ().c_str ()));
        retcode = DDS::RETCODE_ERROR;
      }
    catch (...)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_Handler::get_participant_qos - ")
          ACE_TEXT ("Unexpected exception whilst parsing XML into IDL.\n")));
        retcode = DDS::RETCODE_ERROR;
      }

    return retcode;
  }

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
