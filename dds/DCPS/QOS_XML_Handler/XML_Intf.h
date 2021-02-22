//==============================================================
/**
 *  @file  XML_Intf.h
 *
 *
 *  @author Danilo C. Zanellla (dczanella@gmail.com)
 */
//================================================================

#ifndef DCPS_CONFIG_XML_INTF_H
#define DCPS_CONFIG_XML_INTF_H
#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/QOS_XML_Handler/XML_QOS_Handler_Export.h"

namespace XML
{
  class XML_Typedef;
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class XML_QOS_Handler_Export QOS_XML_Handler
  {
  public:
    QOS_XML_Handler (void);

    virtual ~QOS_XML_Handler (void);

    /**
     *
     * init
     *
     * The init method interface method. It will validate
     * it against the schema. It returns RETCODE_ERROR
     * when any error occurs during parsing
     *
     */
	virtual DDS::ReturnCode_t
    init (const ACE_TCHAR * file) = 0;


    //@{
    /**
     *
     * These methods will search for the profileQos in
     * profiles_, using the given profile_name.
     * If found, these methods will invoke
     * the corresponding method on the corresponding Handler
     * class.
     * These classes are available in the
     * xxxQos_Handler.h files.
     *
     */
    virtual DDS::ReturnCode_t
    get_datawriter_qos (::DDS::DataWriterQos& dw_qos,
                        const char * profile_name,
                        const char * topic_name);

    virtual DDS::ReturnCode_t
    get_datareader_qos (::DDS::DataReaderQos& dr_qos,
                        const char * profile_name,
                        const char * topic_name);

    virtual DDS::ReturnCode_t
    get_topic_qos (::DDS::TopicQos& tp_qos,
                    const char * profile_name,
                    const char * topic_name);

    virtual DDS::ReturnCode_t
    get_publisher_qos (::DDS::PublisherQos& pub_qos,
                        const char * profile_name);

    virtual DDS::ReturnCode_t
    get_subscriber_qos (::DDS::SubscriberQos& sub_qos,
                        const char * profile_name);

    virtual DDS::ReturnCode_t
    get_participant_qos (::DDS::DomainParticipantQos& sub_qos,
                          const char * profile_name);
    //@}

    /**
     *
     * add_search_path will add a relative path to the XML
     * parsing library. The XML parsing library will use
     * this path to search for the schema
     *
     */
    virtual void
    add_search_path (const ACE_TCHAR *environment,
                      const ACE_TCHAR *relpath) = 0;

  protected:
    ::dds::qosProfile_seq profiles_;

    /**
     *
     * Searches for the profile in the XML file, using the given
     * profile name.
     *
     */
    virtual ::dds::qosProfile * get_profile (const char * profile_name);
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DCPS_CONFIG_XML_INTF_H */
