/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MONITOR_FACTORY_BASE_H
#define OPENDDS_DCPS_MONITOR_FACTORY_BASE_H

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"
#include "tao/corba.h"
#include "dcps_export.h"
#include "dds/DCPS/PublicationInstance.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Service_Participant;
class DomainParticipantImpl;
class TopicImpl;
class PublisherImpl;
class SubscriberImpl;
class DataWriterImpl;
class DataReaderImpl;
class TransportImpl;


class Monitor {
public:
  Monitor() { }
  virtual ~Monitor() { }
  virtual void report() = 0;
};

/**
* @class MonitorFactory
*
* @brief Null implementation of the MonitorFactory
*
* Full version of this library is implemented by the monitor lib.
*/
class OpenDDS_Dcps_Export MonitorFactory
  : public ACE_Service_Object {
public:
  MonitorFactory();

  virtual ~MonitorFactory();

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

  ///Initialize the monitor (required to report data)
  virtual void initialize();

  static int service_initialize();
};

static int OpenDDS_Requires_MonitorFactory_Initializer =
  MonitorFactory::service_initialize();

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

ACE_STATIC_SVC_DECLARE (MonitorFactory)
ACE_FACTORY_DECLARE (OpenDDS_Dcps, MonitorFactory)

#endif /* OPENDDS_DCPS_MONITOR_FACTORY_BASE_H */
