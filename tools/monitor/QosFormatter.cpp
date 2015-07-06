/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "QosFormatter.h"

#include <QtCore/QObject>

#include "dds/DdsDcpsInfrastructureC.h"

template<typename QosType>
QString
QosToQString( const QosType& /* value */)
{
  // It is an error to convert something we have not written a conversion
  // for.
  return QString( QObject::tr("<formatting-error>"));
}

//    QString - to help simplify calling code.
template<>
QString
QosToQString<QString>( const QString& value)
{
  return value;
}

//    BuiltinTopicKey_t
template<>
QString
QosToQString<DDS::BuiltinTopicKey_t>( const DDS::BuiltinTopicKey_t& value)
{
  return QString("value = [ 0x%1, 0x%2, 0x%3 ]")
         .arg(value.value[ 0], 8, 16, QLatin1Char('0'))
         .arg(value.value[ 1], 8, 16, QLatin1Char('0'))
         .arg(value.value[ 2], 8, 16, QLatin1Char('0'));
}

//    OctetSeq
template<>
QString
QosToQString<DDS::OctetSeq>( const DDS::OctetSeq& value)
{
  QString result = QString("size = %1, value = ").arg(value.length());
  for( unsigned int index = 0; index < value.length(); ++index) {
    QChar c( value[index]);
    if( c.isPrint()) {
      result.append( QString("'%1',").arg(value[index]));
    } else {
      result.append(
        QString("0x%1,")
        .arg(static_cast<int>(value[index]), 2, 16, QLatin1Char('0'))
      );
    }
  }
  return result;
}

//    DeadlineQosPolicy
template<>
QString
QosToQString<DDS::DeadlineQosPolicy>( const DDS::DeadlineQosPolicy& value)
{
  return QString("period = %1 S, %2 nS")
         .arg(value.period.sec)
         .arg(value.period.nanosec);
}

//    DestinationOrderQosPolicy
template<>
QString
QosToQString<DDS::DestinationOrderQosPolicy>(
  const DDS::DestinationOrderQosPolicy& value
)
{
  switch( value.kind) {
    case DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS:
      return QString( "RECEPTION TIMESTAMP");

    case DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS:
      return QString( "SOURCE TIMESTAMP");

    default:
      return QString( "<bad value == %1>").arg(value.kind);
  }
}

//    DurabilityQosPolicy
template<>
QString
QosToQString<DDS::DurabilityQosPolicy>( const DDS::DurabilityQosPolicy& value)
{
  switch( value.kind) {
    case DDS::VOLATILE_DURABILITY_QOS:
      return QString( "kind = VOLATILE");

    case DDS::TRANSIENT_LOCAL_DURABILITY_QOS:
      return QString( "kind = TRANSIENT LOCAL");

    case DDS::TRANSIENT_DURABILITY_QOS:
      return QString( "kind = TRANSIENT");

    case DDS::PERSISTENT_DURABILITY_QOS:
      return QString( "kind = PERSISTENT LOCAL");

    default:
      return QString( "<bad value == %1>").arg(value.kind);
  }
}

//    DurabilityServiceQosPolicy
template<>
QString
QosToQString<DDS::DurabilityServiceQosPolicy>(
  const DDS::DurabilityServiceQosPolicy& value
)
{
  QString result = QString("cleanup delay = %1 S, %2 nS, ")
                   .arg(value.service_cleanup_delay.sec)
                   .arg(value.service_cleanup_delay.nanosec);

  switch( value.history_kind) {
    case DDS::KEEP_LAST_HISTORY_QOS:
      result.append( QString("history kind = KEEP LAST, "));
      break;

    case DDS::KEEP_ALL_HISTORY_QOS:
      result.append( QString("history kind = KEEP ALL, "));
      break;

    default:
      result.append( QString( "<bad value == %1>, ").arg(value.history_kind));
      break;
  }

  result.append( QString("history depth = %1, ")
                 .arg(value.history_depth));

  result.append( QString("max samples = %1, ")
                 .arg(value.max_samples));

  result.append( QString("max instances = %1, ")
                 .arg(value.max_instances));

  result.append( QString("max samples per instance = %1.")
                 .arg(value.max_samples_per_instance));

  return result;
}

//    GroupDataQosPolicy
template<>
QString
QosToQString<DDS::GroupDataQosPolicy>( const DDS::GroupDataQosPolicy& value)
{
  return QosToQString( value.value);
}

//    HistoryQosPolicy
template<>
QString
QosToQString<DDS::HistoryQosPolicy>( const DDS::HistoryQosPolicy& value)
{
  QString result = QString("history kind = ");

  switch( value.kind) {
    case DDS::KEEP_LAST_HISTORY_QOS:
      result.append( QString("KEEP LAST"));
      break;

    case DDS::KEEP_ALL_HISTORY_QOS:
      result.append( QString("KEEP ALL"));
      break;

    default:
      result.append( QString( "<bad value == %1>").arg(value.kind));
      break;
  }

  result.append( QString(", depth = %1.")
                 .arg(value.depth));

  return result;
}

//    LatencyBudgetQosPolicy
template<>
QString
QosToQString<DDS::LatencyBudgetQosPolicy>(
  const DDS::LatencyBudgetQosPolicy& value
)
{
  return QString("duration = %1 S, %2 nS")
         .arg(value.duration.sec)
         .arg(value.duration.nanosec);
}

//    LifespanQosPolicy
template<>
QString
QosToQString<DDS::LifespanQosPolicy>( const DDS::LifespanQosPolicy& value)
{
  return QString("duration = %1 S, %2 nS")
         .arg(value.duration.sec)
         .arg(value.duration.nanosec);
}

//    LivelinessQosPolicy
template<>
QString
QosToQString<DDS::LivelinessQosPolicy>( const DDS::LivelinessQosPolicy& value)
{
  QString result = QString("kind = ");

  switch( value.kind) {
    case DDS::AUTOMATIC_LIVELINESS_QOS:
      result.append( QString("AUTOMATIC"));
      break;

    case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
      result.append( QString("MANUAL BY PARTICIPANT"));
      break;

    case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      result.append( QString("MANUAL BY TOPIC"));
      break;

    default:
      result.append( QString( "<bad value == %1>").arg(value.kind));
      break;
  }

  result.append( QString(", lease duration = %1 S, %2 nS")
                 .arg(value.lease_duration.sec)
                 .arg(value.lease_duration.nanosec));

  return result;
}

//    OwnershipQosPolicy
template<>
QString
QosToQString<DDS::OwnershipQosPolicy>( const DDS::OwnershipQosPolicy& value)
{
  switch( value.kind) {
    case DDS::SHARED_OWNERSHIP_QOS:
      return QString( "SHARED");

    case DDS::EXCLUSIVE_OWNERSHIP_QOS:
      return QString( "EXCLUSIVE");

    default:
      return QString( "<bad value == %1>").arg(value.kind);
  }
}

//    OwnershipStrengthQosPolicy
template<>
QString
QosToQString<DDS::OwnershipStrengthQosPolicy>(
  const DDS::OwnershipStrengthQosPolicy& value
)
{
  return QString("value = %1").arg(value.value);
}

//    PartitionQosPolicy
template<>
QString
QosToQString<DDS::PartitionQosPolicy>( const DDS::PartitionQosPolicy& value)
{
  /// @NOTE: The calling code should format each individual name value.
  return QString("[ size = %1 ]").arg(value.name.length());
}

//    PresentationQosPolicy
template<>
QString
QosToQString<DDS::PresentationQosPolicy>(
  const DDS::PresentationQosPolicy& value
)
{
  QString result = QString("scope = ");

  switch( value.access_scope) {
    case DDS::INSTANCE_PRESENTATION_QOS:
      result.append( QString("INSTANCE"));
      break;

    case DDS::TOPIC_PRESENTATION_QOS:
      result.append( QString("TOPIC"));
      break;

    case DDS::GROUP_PRESENTATION_QOS:
      result.append( QString("GROUP"));
      break;

    default:
      result.append( QString( "<bad value == %1>").arg(value.access_scope));
      break;
  }

  result.append( QString(", coherent = %1")
                 .arg(value.coherent_access? "true": "false"));

  result.append( QString(", ordered = %1")
                 .arg(value.ordered_access? "true": "false"));

  return result;
}

//    ReliabilityQosPolicy
template<>
QString
QosToQString<DDS::ReliabilityQosPolicy>( const DDS::ReliabilityQosPolicy& value)
{
  QString result = QString("kind = ");

  switch( value.kind) {
    case DDS::BEST_EFFORT_RELIABILITY_QOS:
      result.append( QString("BEST EFFORT"));
      break;

    case DDS::RELIABLE_RELIABILITY_QOS:
      result.append( QString("RELIABLE"));
      break;

    default:
      result.append( QString( "<bad value == %1>").arg(value.kind));
      break;
  }

  result.append( QString(", max blocking time = %1 S, %2 nS")
                 .arg(value.max_blocking_time.sec)
                 .arg(value.max_blocking_time.nanosec));

  return result;
}

//    ResourceLimitsQosPolicy
template<>
QString
QosToQString<DDS::ResourceLimitsQosPolicy>(
  const DDS::ResourceLimitsQosPolicy& value
)
{
  QString result = QString("max samples = %1, ")
                   .arg(value.max_samples);

  result.append( QString("max instances = %1, ")
                 .arg(value.max_instances));

  result.append( QString("max samples per instance = %1.")
                 .arg(value.max_samples_per_instance));

  return result;
}

//    TimeBasedFilterQosPolicy
template<>
QString
QosToQString<DDS::TimeBasedFilterQosPolicy>(
  const DDS::TimeBasedFilterQosPolicy& value
)
{
  return QString("minimum separation = %1 S, %2 nS")
         .arg(value.minimum_separation.sec)
         .arg(value.minimum_separation.nanosec);
}

//    TopicDataQosPolicy
template<>
QString
QosToQString<DDS::TopicDataQosPolicy>( const DDS::TopicDataQosPolicy& value)
{
  return QosToQString( value.value);
}

//    TransportPriorityQosPolicy
template<>
QString
QosToQString<DDS::TransportPriorityQosPolicy>(
  const DDS::TransportPriorityQosPolicy& value
)
{
  return QString("value = %1").arg(value.value);
}

//    UserDataQosPolicy user_data;
template<>
QString
QosToQString<DDS::UserDataQosPolicy>( const DDS::UserDataQosPolicy& value)
{
  return QosToQString( value.value);
}


