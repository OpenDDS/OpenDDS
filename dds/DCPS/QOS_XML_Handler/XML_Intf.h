//==============================================================
/**
 *  @file  XML_Intf.h
 *
 *
 *  @author Danilo C. Zanella (dczanella@gmail.com)
 */
//================================================================

#ifndef OPENDDS_DCPS_QOS_XML_HANDLER_XML_INTF_H
#define OPENDDS_DCPS_QOS_XML_HANDLER_XML_INTF_H
#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"
#include "OpenDDS_XML_QOS_Handler_Export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class OpenDDS_XML_QOS_Handler_Export QOS_XML_Handler
  {
  public:
    QOS_XML_Handler(void);

    ~QOS_XML_Handler(void);

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
    DDS::ReturnCode_t
    get_datawriter_qos(::DDS::DataWriterQos& dw_qos,
                       const ACE_TCHAR * profile_name,
                       const ACE_TCHAR * topic_name);

    DDS::ReturnCode_t
    get_datareader_qos(::DDS::DataReaderQos& dr_qos,
                       const ACE_TCHAR * profile_name,
                       const ACE_TCHAR * topic_name);

    DDS::ReturnCode_t
    get_topic_qos(::DDS::TopicQos& tp_qos,
                  const ACE_TCHAR * profile_name,
                  const ACE_TCHAR * topic_name);

    DDS::ReturnCode_t
    get_publisher_qos(::DDS::PublisherQos& pub_qos,
                      const ACE_TCHAR * profile_name);

    DDS::ReturnCode_t
    get_subscriber_qos(::DDS::SubscriberQos& sub_qos,
                       const ACE_TCHAR * profile_name);

    DDS::ReturnCode_t
    get_participant_qos(::DDS::DomainParticipantQos& sub_qos,
                        const ACE_TCHAR * profile_name);

    /**
     *  Add qos profile to sequence
     */
    DDS::ReturnCode_t
    addQoSProfile(const dds::qosProfile & profile);

    /**
     *  Add qos profiles to sequence
     */
    DDS::ReturnCode_t
    addQoSProfileSeq(const dds::qosProfile_seq & profiles);

    /**
     *  Remove  qos profile to sequence
     */
    DDS::ReturnCode_t
    delQoSProfile(const ACE_TCHAR * profileName);

    /**
     *  Get number of profiles in sequence
     */
    size_t length() const;

    /**
    * Get profile by name
    */
    ::dds::qosProfile getProfile(const ACE_TCHAR * profileName) {
      return *get_profile (profileName);
    }

    /**
     *  get profiles sequence
     */
    const ::dds::qosProfile_seq& get() {
      return profiles_;
    }

  protected:
    ::dds::qosProfile_seq profiles_;

    /**
     *
     * Searches for the profile in the XML file, using the given
     * profile name.
     *
     */
    ::dds::qosProfile * get_profile(const ACE_TCHAR * profile_name);
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DCPS_CONFIG_XML_INTF_H */
