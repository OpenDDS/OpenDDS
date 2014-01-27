// $Id$

#include "QOS_Common.h"

void
QosCommon::get_durability_kind (const ::dds::durabilityKind kind,
                                ::DDS::DurabilityQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::durabilityKind::VOLATILE_DURABILITY_QOS_l:
      dds_kind = ::DDS::VOLATILE_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::TRANSIENT_LOCAL_DURABILITY_QOS_l:
      dds_kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::TRANSIENT_DURABILITY_QOS_l:
      dds_kind = ::DDS::TRANSIENT_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::PERSISTENT_DURABILITY_QOS_l:
      dds_kind = ::DDS::PERSISTENT_DURABILITY_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_durability_kind - ")
        ACE_TEXT ("Unknown durability kind found <%d>; returning VOLATILE_DURABILITY_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::VOLATILE_DURABILITY_QOS;
      break;
    }
}

void
QosCommon::get_history_kind (const ::dds::historyKind kind,
                             ::DDS::HistoryQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::historyKind::KEEP_ALL_HISTORY_QOS_l:
      dds_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      break;
    case ::dds::historyKind::KEEP_LAST_HISTORY_QOS_l:
      dds_kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_history_kind - ")
        ACE_TEXT ("Unknown history kind found <%d>; returning KEEP_ALL_HISTORY_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      break;
    }
}

void
QosCommon::get_duration (::DDS::Duration_t& duration, const ACE_TCHAR * sec, const ACE_TCHAR * nsec)
{
  if (ACE_OS::strcmp (sec, "DURATION_INFINITY") == 0 ||
      ACE_OS::strcmp (sec, "DURATION_INFINITE_SEC") == 0)
    {
      duration.sec = 0x7fffffff;
    }
  else
    {
      duration.sec = ACE_OS::atoi (sec);
    }

  if (ACE_OS::strcmp (nsec, "DURATION_INFINITY") == 0 ||
      ACE_OS::strcmp (nsec, "DURATION_INFINITE_NSEC") == 0)
    {
      duration.nanosec =  0x7fffffff;
    }
  else
    {
      duration.nanosec = ACE_OS::atoi (nsec);
    }
}

::CORBA::Long
QosCommon::get_qos_long(const ACE_TCHAR* value)
{
  if (ACE_OS::strcmp (value, "LENGTH_UNLIMITED") == 0)
    {
      return -1;
    }
  else
    {
      return atoi (value);
    }
}

void
QosCommon::get_liveliness_kind (const ::dds::livelinessKind kind,
                                ::DDS::LivelinessQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::livelinessKind::AUTOMATIC_LIVELINESS_QOS_l:
      dds_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      break;
    case ::dds::livelinessKind::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS_l:
      dds_kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      break;
    case ::dds::livelinessKind::MANUAL_BY_TOPIC_LIVELINESS_QOS_l:
      dds_kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_liveliness_kind - ")
        ACE_TEXT ("Unknown liveliness kind found <%d>; returning AUTOMATIC_LIVELINESS_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      break;
    }
}

void
QosCommon::get_realiability_kind (const ::dds::reliabilityKind kind,
                                  ::DDS::ReliabilityQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::reliabilityKind::BEST_EFFORT_RELIABILITY_QOS_l:
      dds_kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case ::dds::reliabilityKind::RELIABLE_RELIABILITY_QOS_l:
      dds_kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_liveliness_kind - ")
        ACE_TEXT ("Unknown reliability kind found <%d>; returning BEST_EFFORT_RELIABILITY_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    }
}

void
QosCommon::get_destination_order_kind (const ::dds::destinationOrderKind kind,
                                       ::DDS::DestinationOrderQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::destinationOrderKind::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS_l:
      dds_kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    case ::dds::destinationOrderKind::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS_l:
      dds_kind = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_destination_order_kind - ")
        ACE_TEXT ("Unknown destination order kind found <%d>; returning BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    }
}

void
QosCommon::get_ownership_kind (const ::dds::ownershipKind kind,
                               ::DDS::OwnershipQosPolicyKind& dds_kind)
{
  switch (kind.integral ())
    {
    case ::dds::ownershipKind::SHARED_OWNERSHIP_QOS_l:
      dds_kind = ::DDS::SHARED_OWNERSHIP_QOS;
      break;
    case ::dds::ownershipKind::EXCLUSIVE_OWNERSHIP_QOS_l:
      dds_kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
      break;
    default:
      ACE_ERROR ((LM_DEBUG,
        ACE_TEXT ("QosCommon::get_ownership_kind - ")
        ACE_TEXT ("Unknown ownership kind found <%d>; returning SHARED_OWNERSHIP_QOS\n"),
        kind.integral ()));
      dds_kind = ::DDS::SHARED_OWNERSHIP_QOS;
      break;
    }
}
