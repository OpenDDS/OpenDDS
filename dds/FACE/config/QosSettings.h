#ifndef OPENDDS_QOS_SETTINGS_H
#define OPENDDS_QOS_SETTINGS_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export QosSettings {
  public:
    QosSettings();

    enum QosLevel {
      publisher,
      subscriber,
      data_writer,
      data_reader
    };

    int set_qos(QosLevel level, const char* name, const char* value);

    void apply_to(DDS::PublisherQos&  target) const;
    void apply_to(DDS::SubscriberQos& target) const;
    void apply_to(DDS::DataWriterQos& target) const;
    void apply_to(DDS::DataReaderQos& target) const;

    const DDS::PublisherQos& publisher_qos() const { return publisher_qos_; }
    const DDS::SubscriberQos& subscriber_qos() const { return subscriber_qos_; }
    const DDS::DataWriterQos& data_writer_qos() const { return data_writer_qos_; }
    const DDS::DataReaderQos& data_reader_qos() const { return data_reader_qos_; }

  private:
    // DomainPartipantFactory, DomainParticipant, and Topic qos
    // are not usable in FACE
    DDS::PublisherQos publisher_qos_;
    DDS::SubscriberQos subscriber_qos_;
    DDS::DataWriterQos data_writer_qos_;
    DDS::DataReaderQos data_reader_qos_;

    int set_qos(DDS::PublisherQos& target, const char* name, const char* value);
    int set_qos(DDS::SubscriberQos& target, const char* name, const char* value);
    int set_qos(DDS::DataWriterQos& target, const char* name, const char* value);
    int set_qos(DDS::DataReaderQos& target, const char* name, const char* value);
};

typedef OPENDDS_MAP(OPENDDS_STRING, QosSettings) QosMap;

} } }

#endif
