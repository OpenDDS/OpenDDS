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
#include <memory>
#include <string_view>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class OpenDDS_XML_QOS_Handler_Export QOS_XML_Handler
  {
  public:
    QOS_XML_Handler();

    ~QOS_XML_Handler();

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
                       std::string_view profile_name,
                       std::string_view topic_name);

    DDS::ReturnCode_t
    get_datareader_qos(::DDS::DataReaderQos& dr_qos,
                       std::string_view profile_name,
                       std::string_view topic_name);

    DDS::ReturnCode_t
    get_topic_qos(::DDS::TopicQos& tp_qos,
                  std::string_view profile_name,
                  std::string_view topic_name);

    DDS::ReturnCode_t
    get_publisher_qos(::DDS::PublisherQos& pub_qos,
                      std::string_view profile_name);

    DDS::ReturnCode_t
    get_subscriber_qos(::DDS::SubscriberQos& sub_qos,
                       std::string_view profile_name);

    DDS::ReturnCode_t
    get_participant_qos(::DDS::DomainParticipantQos& sub_qos,
                        std::string_view profile_name);

    size_t length() const;

    /**
    * Get profile by name
    */
    const ::dds::qosProfile* getProfile(std::string_view profileName) const {
      return get_profile(profileName);
    }

    /**
     *  get profiles sequence
     */
    const ::dds::qosProfile_seq& get() const {
      return *profiles_;
    }

  protected:
    void set_profiles(::dds::qosProfile_seq* profiles) {
      profiles_.reset(profiles);
    }

    std::unique_ptr<::dds::qosProfile_seq> profiles_;

    /**
     *
     * Searches for the profile in the XML file, using the given
     * profile name.
     *
     */
    const ::dds::qosProfile * get_profile(std::string_view profile_name) const;
  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DCPS_CONFIG_XML_INTF_H */
