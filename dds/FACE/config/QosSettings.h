#ifndef OPENDDS_QOS_SETTINGS_H
#define OPENDDS_QOS_SETTINGS_H

#include <map>
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Definitions.h"

namespace OpenDDS { namespace FACE { namespace config {

class QosSettings {
  public:
    QosSettings();

    enum QosLevel {
      publisher,
      subscriber,
      data_writer,
      data_reader
    };


    void apply_to(DDS::PublisherQos&  target) const;
    void apply_to(DDS::SubscriberQos& target) const;
    void apply_to(DDS::DataWriterQos& target) const;
    void apply_to(DDS::DataReaderQos& target) const;

    static void parse(FILE* configFile, QosSettings& settings);

  private:
    // DomainPartipantFactory, DomainParticipant, and Topic qos 
    // are not usable in FACE
    DDS::PublisherQos publisher_qos_;
    DDS::SubscriberQos subscriber_qos_;
    DDS::DataWriterQos data_writer_qos_;
    DDS::DataReaderQos data_reader_qos_;

    void set_qos(QosLevel level, const char* name, const char* value);
    void set_qos(DDS::PublisherQos& target, const char* name, const char* value);
    void set_qos(DDS::SubscriberQos& target, const char* name, const char* value);
    void set_qos(DDS::DataWriterQos& target, const char* name, const char* value);
    void set_qos(DDS::DataReaderQos& target, const char* name, const char* value);
};

typedef OPENDDS_MAP(OPENDDS_STRING, QosSettings) QosMap;

} } }

#endif
