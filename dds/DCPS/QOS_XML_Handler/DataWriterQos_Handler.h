/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 */
#ifndef OPENDDS_DCPS_QOS_XML_HANDLER_DATAWRITERQOS_HANDLER_H
#define OPENDDS_DCPS_QOS_XML_HANDLER_DATAWRITERQOS_HANDLER_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

class DataWriterQos_Handler
{
public:
  /**
   * Find the correct datawriterQos within the given profile,
   * based on the given name.
   */
  static bool get_datawriter_qos (::DDS::DataWriterQos& dw_qos,
                                  ::dds::qosProfile * profile,
                                  const ACE_TCHAR * name = 0);
private:
  /**
   * Start parsing the QOS XML, using the template classes.
   */
  static bool get_datawriter_qos (DDS::DataWriterQos& dw_qos,
                                  dds::datawriterQos * dw);
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"
#endif /* DATAWRITER_QOS_HANDLER_H */
