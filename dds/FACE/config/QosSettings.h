#ifndef OPENDDS_QOS_SETTINGS_H
#define OPENDDS_QOS_SETTINGS_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export QosSettings {
  public:
    QosSettings();

    enum QosLevel {
      publisher,
      subscriber,
      datawriter,
      datareader
    };

    int set_qos(QosLevel level, const char* name, const char* value);

    void apply_to(DDS::PublisherQos&  target) const;
    void apply_to(DDS::SubscriberQos& target) const;
    void apply_to(DDS::DataWriterQos& target) const;
    void apply_to(DDS::DataReaderQos& target) const;

    DDS::PublisherQos& publisher_qos() { return publisher_qos_; }
    DDS::SubscriberQos& subscriber_qos() { return subscriber_qos_; }
    DDS::DataWriterQos& datawriter_qos() { return datawriter_qos_; }
    DDS::DataReaderQos& datareader_qos() { return datareader_qos_; }

  private:
    // DomainPartipantFactory, DomainParticipant, and Topic qos
    // are not usable in FACE
    DDS::PublisherQos publisher_qos_;
    DDS::SubscriberQos subscriber_qos_;
    DDS::DataWriterQos datawriter_qos_;
    DDS::DataReaderQos datareader_qos_;

    int set_qos(DDS::PublisherQos& target, const char* name, const char* value);
    int set_qos(DDS::SubscriberQos& target, const char* name, const char* value);
    int set_qos(DDS::DataWriterQos& target, const char* name, const char* value);
    int set_qos(DDS::DataReaderQos& target, const char* name, const char* value);
};

typedef OPENDDS_MAP(OPENDDS_STRING, QosSettings) QosMap;

} } }

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
