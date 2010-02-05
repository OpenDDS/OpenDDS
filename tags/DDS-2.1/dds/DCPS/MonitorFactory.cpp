/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "MonitorFactory.h"

namespace OpenDDS {
namespace DCPS {

// Implementation skeleton constructor
MonitorFactory::MonitorFactory()
{
}

// Implementation skeleton destructor
MonitorFactory::~MonitorFactory()
{
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_sp_monitor(Service_Participant*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_dp_monitor(DomainParticipantImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_topic_monitor(TopicImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_publisher_monitor(PublisherImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_subscriber_monitor(SubscriberImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_data_writer_monitor(DataWriterImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_data_writer_periodic_monitor(DataWriterImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_data_reader_monitor(DataReaderImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_data_reader_periodic_monitor(DataReaderImpl*)
{
  return 0;
}

OpenDDS::DCPS::Monitor*
MonitorFactory::create_transport_monitor(TransportImpl*)
{
  return 0;
}

void
MonitorFactory::initialize()
{
}

} // namespace DCPS
} // namespace OpenDDS

using namespace OpenDDS::DCPS;

