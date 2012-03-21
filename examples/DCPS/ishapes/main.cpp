#include <ShapesDialog.hpp>
#include <QtGui/QApplication>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <list>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst_rch.h>

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[]) {
  int retval = -1;

  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    TransportConfig_rch config =
      TransportRegistry::instance()->create_config("rtps_interop_demo");
    TransportInst_rch inst =
      TransportRegistry::instance()->create_inst("the_rtps_transport",
                                                 "rtps_udp");
    RtpsUdpInst_rch rui = static_rchandle_cast<RtpsUdpInst>(inst);
    rui->handshake_timeout_ = 1;

    config->instances_.push_back(inst);
    TransportRegistry::instance()->global_config(config);

    DDS::DomainId_t domain = 0;
    bool multicast = true;
    unsigned int resend = 1;
    std::string partition;

    if (argc > 1) {
      domain = ACE_OS::atoi (argv[1]);
      std::cout << "Connecting to domain: " << domain << std::endl;
    }

    RtpsDiscovery_rch disc = new RtpsDiscovery("RtpsDiscovery");
    for (int curr = 2; curr < argc; ++curr) {
      if (ACE_OS::strcmp("-u", argv[curr]) == 0) {
        multicast = false;
        std::cout << "SEDP unicast only" << std::endl;
      }
      else if ((ACE_OS::strcmp("-r", argv[curr]) == 0) && (curr + 1 < argc)) {
        resend = ACE_OS::atoi(argv[++curr]);
        std::cout << "Resend: " << resend << " sec" << std::endl;
      }
      else if ((ACE_OS::strcmp("-pb", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "pb: " << temp << std::endl;
        disc->pb(temp);
      }
      else if ((ACE_OS::strcmp("-dg", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "dg: " << temp << std::endl;
        disc->dg(temp);
      }
      else if ((ACE_OS::strcmp("-pg", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "pg: " << temp << std::endl;
        disc->pg(temp);
      }
      else if ((ACE_OS::strcmp("-d0", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "d0: " << temp << std::endl;
        disc->d0(temp);
      }
      else if ((ACE_OS::strcmp("-d1", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "d1: " << temp << std::endl;
        disc->d1(temp);
      }
      else if ((ACE_OS::strcmp("-dx", argv[curr]) == 0) && (curr + 1 < argc)) {
        const u_short temp = ACE_OS::atoi(argv[++curr]);
        std::cout << "dx: " << temp << std::endl;
        disc->dx(temp);
      }
      else if ((ACE_OS::strcmp("-partition", argv[curr]) == 0) &&
               (curr + 1 < argc)) {
        partition = argv[++curr];
        std::cout << "Partition[0]: " << partition << std::endl;
      }
      else {
        std::cout << "Ignoring unkown param: " << argv[curr] << std::endl;
      }
    }

    disc->resend_period(ACE_Time_Value(resend));
    disc->sedp_multicast(multicast);
    TheServiceParticipant->add_discovery(static_rchandle_cast<Discovery>(disc));
    TheServiceParticipant->set_repo_domain(domain, disc->key());

    // If the default RTPS discovery object has not been configured,
    // instantiate it now.
    const Service_Participant::RepoKeyDiscoveryMap& discoveryMap =
      TheServiceParticipant->discoveryMap();
    if (discoveryMap.find(Discovery::DEFAULT_RTPS) == discoveryMap.end()) {
      RtpsDiscovery_rch discovery = new RtpsDiscovery(Discovery::DEFAULT_RTPS);
      TheServiceParticipant->add_discovery(static_rchandle_cast<Discovery>(discovery));
    }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(domain,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    if (!participant) {
      std::cerr << "Could not connect to domain " << std::endl;
    }

    srand(clock());
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(ishape);
    // create and show your widgets here
    ShapesDialog shapes(participant, partition);
    shapes.show();
    retval = app.exec();

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

  } catch (const CORBA::Exception& e) {
    std::cerr << "Exception in main: " << e << std::endl;
    retval = -1;
  }
  TheServiceParticipant->shutdown();

  return retval;
}

