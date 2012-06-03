/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 * $Id$
 *
 */
#ifndef DATAREADER_QOS_HANDLER_H
#define DATAREADER_QOS_HANDLER_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"

class DataReaderQos_Handler
{
public:
  /**
   * Find the correct datareaderQos within the given profile,
   * based on the given name.
   */
  static bool get_datareader_qos (::DDS::DataReaderQos& dr_qos,
                                  ::dds::qosProfile * profile,
                                  const ACE_TCHAR * name = 0);
private:
  /**
   * Start parsing the QOS XML, using the template classes.
   */
  static bool get_datareader_qos (DDS::DataReaderQos& dr_qos,
                                  dds::datareaderQos * dr);
};

#include /**/ "ace/post.h"
#endif /* DATAREADER_QOS_HANDLER_H */
