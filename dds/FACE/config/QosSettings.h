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
      data_writer,
      subscriber,
      data_reader
    };

    void set(QosLevel level, const char* name, const char* value);

    void apply_to(DDS::PublisherQos&  publisher_qos) const;
    void apply_to(DDS::DataWriterQos& writer_qos) const;
    void apply_to(DDS::SubscriberQos& subscriber_qos) const;
    void apply_to(DDS::DataReaderQos& reader_qos) const;

  private:
    // DomainPartipantFactory, DomainParticipant, and Topic qos 
    // are not usable in FACE
    DDS::PublisherQos publisher_qos_;
    DDS::DataWriterQos writer_qos_;

    DDS::SubscriberQos subscriber_qos_;
    DDS::DataReaderQos reader_qos_;

};

typedef OPENDDS_MAP(OPENDDS_STRING, QosSettings) QosMap;

} } }

#endif
