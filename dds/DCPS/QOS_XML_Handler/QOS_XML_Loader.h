/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * Starting point for QOS XML parsing library.
 *
 */
#ifndef QOS_XML_LOADER_H
#define QOS_XML_LOADER_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/QOS_XML_Handler/XML_QOS_Handler_Export.h"
#include "dds/DCPS/QOS_XML_Handler/XML_File_Intf.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class XML_QOS_Handler_Export QOS_XML_Loader
  {
  public:
    QOS_XML_Loader (void);
    ~QOS_XML_Loader (void);

    /**
     * init
     *
     * qos_profile profile should be formatted like:
     *
     *    qos_base_file_name_without_extension#profile_name_in_xml_file
     *
     * Init parses this string and will append ".xml" to
     * qos_base_file_name_without_extension. It'll than invoke
     * the init method on the XML_File_Intf class.
     *
     */
    DDS::ReturnCode_t
    init (const ACE_TCHAR * qos_profile);

    //@{
    /**
     *
     * These methods have DDS QOS, a profile and a topic_name as
     * input.
     *
     * The DDS QOS is passed by reference. This library fills this
     * QOS and will return it to the callee.
     *
     * qos_profile should be the same profile string as the one
     * passed to the init method.
     *
     * If the profile name is correct, the methods will invoke
     * the corresponding method in the XML_File_Intf class.
     *
     */
    DDS::ReturnCode_t
    get_datawriter_qos (DDS::DataWriterQos& dw_qos,
                        const ACE_TCHAR *qos_profile,
                        const ACE_TCHAR* topic_name);

    DDS::ReturnCode_t
    get_datareader_qos (DDS::DataReaderQos& dr_qos,
                        const ACE_TCHAR *qos_profile,
                        const ACE_TCHAR* topic_name);

    DDS::ReturnCode_t
    get_publisher_qos (DDS::PublisherQos& pub_qos,
                        const ACE_TCHAR *qos_profile);

    DDS::ReturnCode_t
    get_subscriber_qos (DDS::SubscriberQos& sub_qos,
                        const ACE_TCHAR *qos_profile);

    DDS::ReturnCode_t
    get_topic_qos (DDS::TopicQos& topic_qos,
                   const ACE_TCHAR *qos_profile,
                   const ACE_TCHAR *topic_name);

    DDS::ReturnCode_t
    get_participant_qos (DDS::DomainParticipantQos& part_qos,
                         const ACE_TCHAR *qos_profile);
    //@}

  private:
    QOS_XML_File_Handler xml_file_;

    ACE_TString get_xml_file_name(const ACE_TCHAR* qos_profile);
    ACE_TString get_profile_name(const ACE_TCHAR* qos_profile);
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* QOS_XML_LOADER_H */
