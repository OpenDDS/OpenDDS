/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 * $Id$
 *
 * Converting most common QOS XML settings to IDL QOS.
 *
 */
#ifndef QOS_COMMON_H
#define QOS_COMMON_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds_qos.hpp"

class QosCommon
{
public:

  //@{
  /** Operations which convert QOS XML kinds to QOS IDL kinds. */

  static void get_durability_kind (const ::dds::durabilityKind kind,
                                   ::DDS::DurabilityQosPolicyKind& dds_kind);
  static void get_history_kind (const ::dds::historyKind kind,
                                ::DDS::HistoryQosPolicyKind& dds_kind);
  static void get_liveliness_kind (const ::dds::livelinessKind kind,
                                   ::DDS::LivelinessQosPolicyKind& dds_kind);
  static void get_realiability_kind (const dds::reliabilityKind kind,
                                     ::DDS::ReliabilityQosPolicyKind& dds_kind);
  static void get_destination_order_kind (const ::dds::destinationOrderKind kind,
                                          ::DDS::DestinationOrderQosPolicyKind& dds_kind);

  static void get_ownership_kind (::dds::ownershipKind kind,
                                  ::DDS::OwnershipQosPolicyKind& dds_kind);
  //@}

  /**
   * get_duration
   *
   * The QOS XML seconds and nano seconds tags may contain text
   * (DURATION_INFINITY, DURATION_INFINITY_SEC, DURATION_INFINITY_NSEC). These
   * values need to be converted to a numeric value.
   *
   */
  static void get_duration (::DDS::Duration_t& duration, const ACE_TCHAR * sec, const ACE_TCHAR * nsec);

  /**
   * get_qos_long
   *
   * The QOS XML integer tags may contain text (LENGTH_UNLIMITED).
   * This value need to be converted to a numeric value.
   *
   */
  static ::CORBA::Long get_qos_long (const ACE_TCHAR * value);

};

#include /**/ "ace/post.h"
#endif /* QOS_COMMON_H */
