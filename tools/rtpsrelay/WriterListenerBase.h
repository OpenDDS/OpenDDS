#ifndef RTPSRELAY_WRITER_LISTENER_BASE_H_
#define RTPSRELAY_WRITER_LISTENER_BASE_H_

#include <dds/DdsDcpsPublicationC.h>

namespace RtpsRelay {

class WriterListenerBase : public DDS::DataWriterListener {
public:
  void on_publication_matched(DDS::DataWriter_ptr, const DDS::PublicationMatchedStatus&) override {}
  void on_offered_deadline_missed(DDS::DataWriter_ptr, const DDS::OfferedDeadlineMissedStatus&) override {}
  void on_offered_incompatible_qos(DDS::DataWriter_ptr, const DDS::OfferedIncompatibleQosStatus&) override {}
  void on_liveliness_lost(DDS::DataWriter_ptr, const DDS::LivelinessLostStatus&) override {}
};

}

#endif // RTPSRELAY_WRITER_LISTENER_BASE_H_
