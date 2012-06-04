/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 * $Id$
 *
 */
#ifndef PARTICIPANT_QOS_HANDLER_H
#define PARTICIPANT_QOS_HANDLER_H

#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "dds/DdsDcpsInfrastructureC.h"

class ParticipantQos_Handler
{
public:
  /**
   * Find the correct domainparticipantQos within the given profile,
   * based on the given name.
   */
  static bool get_participant_qos (::DDS::DomainParticipantQos& dp_qos,
                                  ::dds::qosProfile * profile,
                                  const ACE_TCHAR * name = 0);
private:
  /**
   * Start parsing the QOS XML, using the template classes.
   */
  static bool get_participant_qos (DDS::DomainParticipantQos& dp_qos,
                                  dds::domainparticipantQos * dp);
};

#include /**/ "ace/post.h"
#endif /* PARTICIPANT_QOS_HANDLER_H */
