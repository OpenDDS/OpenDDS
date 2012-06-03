/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 * $Id$
 *
 */
#ifndef SUBSCRIBER_QOS_HANDLER_H
#define SUBSCRIBER_QOS_HANDLER_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"

class SubscriberQos_Handler
{
public:
  /**
   * Find the correct subscriberQos within the given profile,
   * based on the given name.
   */
  static bool get_subscriber_qos (::DDS::SubscriberQos& sub_qos,
                                  ::dds::qosProfile * profile,
                                  const ACE_TCHAR * name = 0);
private:
  /**
   * Start parsing the QOS XML, using the template classes.
   */
  static bool get_subscriber_qos (DDS::SubscriberQos& sub_qos,
                                  dds::subscriberQos * sub);
};

#include /**/ "ace/post.h"
#endif /* SUBSCRIBER_QOS_HANDLER_H */
