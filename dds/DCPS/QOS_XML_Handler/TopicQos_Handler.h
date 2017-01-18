/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 */
#ifndef TOPIC_QOS_HANDLER_H
#define TOPIC_QOS_HANDLER_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/QOS_XML_Handler/XML_QOS_Handler_Export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

class XML_QOS_Handler_Export TopicQos_Handler
{
public:
  /**
   * Find the correct topicQos within the given profile,
   * based on the given name.
   */
  static bool get_topic_qos (::DDS::TopicQos& tp_qos,
                                  ::dds::qosProfile * profile,
                                  const ACE_TCHAR * name = 0);
private:
  /**
   * Start parsing the QOS XML, using the template classes.
   */
  static bool get_topic_qos (DDS::TopicQos& tp_qos,
                                  dds::topicQos * tp);
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"
#endif /* TOPIC_QOS_HANDLER_H */
