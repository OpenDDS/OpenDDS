//
// $Id: CorbaDpShutdown.h 2179 2013-05-28 22:16:51Z mesnierp $
//

#ifndef CORBADPSHUTDOWN_H
#define CORBADPSHUTDOWN_H
#include /**/ "ace/pre.h"

#include "TestS.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>

#ifndef DDS_HAS_MINIMUM_BIT
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include "dds/DCPS/StaticIncludes.h"

static int DOMAIN_ID = 4;

/// Implement the Test::CorbaDpShutdown interface
class CorbaDpShutdown
  : public virtual POA_Test::CorbaDpShutdown
{
public:
  /// Constructor
  CorbaDpShutdown (CORBA::ORB_ptr orb);

  virtual void shutdown (void);

  virtual void start(void);

  virtual void stop(void);

private:
  /// Use an ORB reference to convert strings to objects and shutdown
  /// the application.
  CORBA::ORB_var orb_;
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;
};

#include /**/ "ace/post.h"
#endif /* CORBADPSHUTDOWN_H */
