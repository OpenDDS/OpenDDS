/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

template< typename DataType>
inline
void
MonitorDataStorage::update( const DataType&, bool)
{
  // Error condition.
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::ServiceParticipantReport>(
  const OpenDDS::DCPS::ServiceParticipantReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DomainParticipantReport>(
  const OpenDDS::DCPS::DomainParticipantReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TopicReport>(
  const OpenDDS::DCPS::TopicReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::PublisherReport>(
  const OpenDDS::DCPS::PublisherReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::SubscriberReport>(
  const OpenDDS::DCPS::SubscriberReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterReport>(
  const OpenDDS::DCPS::DataWriterReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataWriterPeriodicReport>(
  const OpenDDS::DCPS::DataWriterPeriodicReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderReport>(
  const OpenDDS::DCPS::DataReaderReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::DataReaderPeriodicReport>(
  const OpenDDS::DCPS::DataReaderPeriodicReport& /* data */,
  bool /* remove */
)
{
}

template<>
inline
void
MonitorDataStorage::update< OpenDDS::DCPS::TransportReport>(
  const OpenDDS::DCPS::TransportReport& /* data */,
  bool /* remove */
)
{
}

