/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_MONITORFACTORYIMPL_H
#define OPENDDS_MONITOR_MONITORFACTORYIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

/**
* @class MonitorFactoryImpl
*
* @brief Full implementation of the MonitorFactoryImpl
*
* Full version of this library is implemented by the monitor lib.
*/
class OpenDDS_monitor_Export MonitorFactoryImpl
  : public DCPS::MonitorFactory {
public:
  MonitorFactoryImpl();

  virtual ~MonitorFactoryImpl();

  /// Factory function to create a service participant monitor object
  virtual DCPS::Monitor* create_sp_monitor(DCPS::Service_Participant* sp);

  /// Factory function to create a domain participant monitor object
  virtual DCPS::Monitor* create_dp_monitor(DCPS::DomainParticipantImpl* dp);

  /// Factory function to create a topic monitor object
  virtual DCPS::Monitor* create_topic_monitor(DCPS::TopicImpl* topic);

  /// Factory function to create a publisher monitor object
  virtual DCPS::Monitor* create_publisher_monitor(DCPS::PublisherImpl* publisher);

  /// Factory function to create a subscriber monitor object
  virtual DCPS::Monitor* create_subscriber_monitor(DCPS::SubscriberImpl* subscriber);

  /// Factory function to create a data writer monitor object
  virtual DCPS::Monitor* create_data_writer_monitor(DCPS::DataWriterImpl* dw);

  /// Factory function to create a data writer periodic monitor object
  virtual DCPS::Monitor* create_data_writer_periodic_monitor(DCPS::DataWriterImpl* dw);

  /// Factory function to create a data reader monitor object
  virtual DCPS::Monitor* create_data_reader_monitor(DCPS::DataReaderImpl* dr);

  /// Factory function to create a data reader periodic monitor object
  virtual DCPS::Monitor* create_data_reader_periodic_monitor(DCPS::DataReaderImpl* dr);

  /// Factory function to create a transport monitor object
  virtual DCPS::Monitor* create_transport_monitor(DCPS::TransportImpl* transport);

  virtual void initialize();

  virtual void deinitialize();

  ServiceParticipantReportDataWriter_ptr get_sp_writer();

  static int service_initialize();

private:
  DDS::DataWriter_ptr create_data_writer(DDS::DomainParticipant_ptr participant,
                                         DDS::Publisher_ptr publisher,
                                         const char* type_name,
                                         const char* topic_name,
                                         const DDS::DataWriterQos& dw_qos);

  DDS::DomainParticipant_var participant_;
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

}
}


ACE_STATIC_SVC_DECLARE (MonitorFactoryImpl)
ACE_FACTORY_DECLARE (OpenDDS_monitor, MonitorFactoryImpl)

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_MONITOR_FACTORY_IMPL_H */
