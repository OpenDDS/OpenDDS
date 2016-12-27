/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MONITOR_FACTORY_IMPL_H
#define OPENDDS_DCPS_MONITOR_FACTORY_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Monitor;
/**
* @class MonitorFactoryImpl
*
* @brief Full implementation of the MonitorFactoryImpl
*
* Full version of this library is implemented by the monitor lib.
*/
class OpenDDS_monitor_Export MonitorFactoryImpl
  : public MonitorFactory {
public:
  MonitorFactoryImpl();

  virtual ~MonitorFactoryImpl();

  /// Factory function to create a service participant monitor object
  virtual Monitor* create_sp_monitor(Service_Participant* sp);

  /// Factory function to create a domain participant monitor object
  virtual Monitor* create_dp_monitor(DomainParticipantImpl* dp);

  /// Factory function to create a topic monitor object
  virtual Monitor* create_topic_monitor(TopicImpl* topic);

  /// Factory function to create a publisher monitor object
  virtual Monitor* create_publisher_monitor(PublisherImpl* publisher);

  /// Factory function to create a subscriber monitor object
  virtual Monitor* create_subscriber_monitor(SubscriberImpl* subscriber);

  /// Factory function to create a data writer monitor object
  virtual Monitor* create_data_writer_monitor(DataWriterImpl* dw);

  /// Factory function to create a data writer periodic monitor object
  virtual Monitor* create_data_writer_periodic_monitor(DataWriterImpl* dw);

  /// Factory function to create a data reader monitor object
  virtual Monitor* create_data_reader_monitor(DataReaderImpl* dr);

  /// Factory function to create a data reader periodic monitor object
  virtual Monitor* create_data_reader_periodic_monitor(DataReaderImpl* dr);

  /// Factory function to create a transport monitor object
  virtual Monitor* create_transport_monitor(TransportImpl* transport);

  virtual void initialize();

  ServiceParticipantReportDataWriter_ptr get_sp_writer();

  static int service_initialize();

private:
  DDS::DataWriter_ptr create_data_writer(DDS::DomainParticipant_ptr participant,
                                         DDS::Publisher_ptr publisher,
                                         const char* type_name,
                                         const char* topic_name,
                                         const DDS::DataWriterQos& dw_qos);

  ServiceParticipantReportDataWriter_var  sp_writer_;
  DomainParticipantReportDataWriter_var   dp_writer_;
  TopicReportDataWriter_var               topic_writer_;
  PublisherReportDataWriter_var           pub_writer_;
  SubscriberReportDataWriter_var          sub_writer_;
  DataWriterReportDataWriter_var          dw_writer_;
  DataWriterPeriodicReportDataWriter_var  dw_per_writer_;
  DataReaderReportDataWriter_var          dr_writer_;
  DataReaderPeriodicReportDataWriter_var  dr_per_writer_;
  TransportReportDataWriter_var           transport_writer_;
};

static int OpenDDS_Requires_MonitorFactoryImpl_Initializer =
  MonitorFactoryImpl::service_initialize();

} // namespace DCPS
} // namespace OpenDDS


ACE_STATIC_SVC_DECLARE (MonitorFactoryImpl)
ACE_FACTORY_DECLARE (OpenDDS_monitor, MonitorFactoryImpl)

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_MONITOR_FACTORY_IMPL_H */
