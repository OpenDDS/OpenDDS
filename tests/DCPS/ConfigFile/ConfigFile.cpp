/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include <dds/DCPS/Service_Participant.h>
#include "ace/Configuration_Import_Export.h"
#include "tao/corba.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/tcp/TcpInst_rch.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/transport/framework/TransportDebug.h"

#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#include "dds/DCPS/RTPS/RtpsDiscovery.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#endif

#include "../common/TestSupport.h"

#include <iostream>

using namespace OpenDDS::DCPS;

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    TEST_CHECK(dpf.in() != 0);

    // From commandline
    TEST_CHECK(TheServiceParticipant->config_store()->get("MY_CONFIG_KEY1", "") == "value1");
    // From environment variable
    TEST_CHECK(TheServiceParticipant->config_store()->get("MY_CONFIG_KEY2", "") == "value2");
    TEST_CHECK(OpenDDS::DCPS::DCPS_debug_level == 1);
    TEST_CHECK(TheServiceParticipant->n_chunks() == 10);
    TEST_CHECK(TheServiceParticipant->association_chunk_multiplier() == 5);
    TEST_CHECK(TheServiceParticipant->liveliness_factor() == 70);
    TEST_CHECK(TheServiceParticipant->bit_transport_port() == 1234);
    TEST_CHECK(TheServiceParticipant->bit_lookup_duration_msec() == 1000);
    TEST_CHECK(TheServiceParticipant->get_BIT() == false);
    TEST_CHECK(OpenDDS::DCPS::Transport_debug_level == 1);
    TEST_CHECK(TheServiceParticipant->pending_timeout() == TimeDuration(10));
    TEST_CHECK(TheServiceParticipant->publisher_content_filter() == false);
    TEST_CHECK(TheServiceParticipant->get_default_discovery() == "MyDefaultDiscovery");
    TEST_CHECK(TheServiceParticipant->default_address() == NetworkAddress(u_short(0), "127.0.0.1"));
    TEST_CHECK(TheServiceParticipant->federation_recovery_duration() == 800);
    TEST_CHECK(TheServiceParticipant->federation_initial_backoff_seconds() == 2);
    TEST_CHECK(TheServiceParticipant->federation_backoff_multiplier() == 3);
    TEST_CHECK(TheServiceParticipant->federation_liveliness() == 70);

    TransportInst_rch inst = TransportRegistry::instance()->get_inst("mytcp");
    TEST_CHECK(inst);

    TcpInst_rch tcp_inst = dynamic_rchandle_cast<TcpInst>(inst);
    TEST_CHECK(tcp_inst);

    // tcp_inst->dump(std::cout);

    TEST_CHECK(tcp_inst->name() == "mytcp");
    TEST_CHECK(tcp_inst->max_packet_size() == 2000000000);
    TEST_CHECK(tcp_inst->max_samples_per_packet() == 3);
    TEST_CHECK(tcp_inst->optimum_packet_size() == 2048);
    TEST_CHECK(tcp_inst->thread_per_connection() == true);
    TEST_CHECK(tcp_inst->datalink_release_delay() == 5000);
    TEST_CHECK(tcp_inst->datalink_control_chunks() == 16);
    TEST_CHECK(tcp_inst->local_address() == "localhost:");
    TEST_CHECK(tcp_inst->enable_nagle_algorithm() == true);
    TEST_CHECK(tcp_inst->conn_retry_initial_delay() == 1000);
    TEST_CHECK(tcp_inst->conn_retry_backoff_multiplier() == 4);
    TEST_CHECK(tcp_inst->conn_retry_attempts() == 4);
    TEST_CHECK(tcp_inst->passive_reconnect_duration() == 4000);
    TEST_CHECK(tcp_inst->max_output_pause_period() == 1000);

    TransportInst_rch inst2 = TransportRegistry::instance()->get_inst("anothertcp");
    TEST_CHECK(inst2);
    TEST_CHECK(inst2->name() == "anothertcp");

    TransportConfig_rch config = TransportRegistry::instance()->get_config("myconfig");
    TEST_CHECK(config);
    TEST_CHECK(config->instances_.size() == 2);
    TEST_CHECK(config->instances_[0] == inst);
    TEST_CHECK(config->instances_[1] == inst2);
    TEST_CHECK(config->swap_bytes_ == true);
    TEST_CHECK(config->passive_connect_duration_ == TimeDuration::from_msec(20000));

    TransportConfig_rch default_config =
#ifdef DDS_HAS_MINIMUM_BIT
      TransportRegistry::instance()->get_config("test1_nobits.ini");
#else
      TransportRegistry::instance()->get_config("test1.ini");
#endif
    TEST_CHECK(default_config);
    //std::cout << "size=" << default_config->instances_.size() << std::endl;
    //for (unsigned int i = 0; i < default_config->instances_.size(); ++i) {
    //  std::cout << "  " << default_config->instances_[i]->name() << std::endl;
    //}
    // Should be in alpha-sorted order
    TEST_CHECK(default_config->instances_.size() == 11);
    TEST_CHECK(default_config->instances_[0] == inst2);  // anothertcp
    TEST_CHECK(default_config->instances_[2] == inst);   // mytcp
    TEST_CHECK(default_config->instances_[9]->name() == std::string("tcp7"));
    TEST_CHECK(default_config->swap_bytes_ == false);
    TEST_CHECK(default_config->passive_connect_duration_ == TimeDuration::from_msec(60000));

    TransportConfig_rch global_config =
      TransportRegistry::instance()->global_config();
    TEST_CHECK(global_config->name() == "myconfig");

    {
      DDS::DomainId_t domain = 1234;
      OpenDDS::DCPS::Discovery::RepoKey key = "333";
      std::string ior = "file://repo2.ior";

      const OpenDDS::DCPS::Service_Participant::DomainRepoMap& domainRepoMap =
        TheServiceParticipant->domainRepoMap();
      TEST_CHECK(domainRepoMap.find(domain) != domainRepoMap.end());
      TEST_CHECK(domainRepoMap.find(domain)->second == key);

      const OpenDDS::DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap =
        TheServiceParticipant->discoveryMap();
      TEST_CHECK(discoveryMap.find(key) != discoveryMap.end());
      OpenDDS::DCPS::Discovery_rch discovery = discoveryMap.find(key)->second;
      TEST_CHECK(static_rchandle_cast<OpenDDS::DCPS::InfoRepoDiscovery>(discovery)
        ->get_stringified_dcps_info_ior() == ior);
    }

    {
      DDS::DomainId_t domain = 1235;
      OpenDDS::DCPS::Discovery::RepoKey key = "xyz";
      std::string ior = "file://repo3.ior";

      const OpenDDS::DCPS::Service_Participant::DomainRepoMap& domainRepoMap =
        TheServiceParticipant->domainRepoMap();
      TEST_CHECK(domainRepoMap.find(domain) != domainRepoMap.end());
      TEST_CHECK(domainRepoMap.find(domain)->second == key);

      const OpenDDS::DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap =
        TheServiceParticipant->discoveryMap();
      TEST_CHECK(discoveryMap.find(key) != discoveryMap.end());
      OpenDDS::DCPS::Discovery_rch discovery = discoveryMap.find(key)->second;
      OpenDDS::DCPS::InfoRepoDiscovery_rch ird =
        dynamic_rchandle_cast<OpenDDS::DCPS::InfoRepoDiscovery>(discovery);
      TEST_CHECK(!ird.is_nil());
      TEST_CHECK(ird->get_stringified_dcps_info_ior() == ior);
      TEST_CHECK(ird->bit_transport_ip() == "1.2.3.4");
      TEST_CHECK(ird->bit_transport_port() == 4321);
    }

#ifndef DDS_HAS_MINIMUM_BIT
    {
      DDS::DomainId_t domain = 21;
      OpenDDS::DCPS::Discovery::RepoKey key = "DEFAULT_RTPS";
      std::string ior = "";

      const OpenDDS::DCPS::Service_Participant::DomainRepoMap& domainRepoMap =
        TheServiceParticipant->domainRepoMap();
      TEST_CHECK(domainRepoMap.find(domain) != domainRepoMap.end());
      TEST_CHECK(domainRepoMap.find(domain)->second == key);

      OpenDDS::DCPS::TransportConfig_rch tconf =
        TransportRegistry::instance()->domain_default_config(domain);
      TEST_CHECK(tconf.is_nil());
    }

    {
      DDS::DomainId_t domain = 99;
      OpenDDS::DCPS::Discovery::RepoKey key = "MyConfig";
      std::string ior = "";

      const OpenDDS::DCPS::Service_Participant::DomainRepoMap& domainRepoMap =
        TheServiceParticipant->domainRepoMap();
      TEST_CHECK(domainRepoMap.find(domain) != domainRepoMap.end());
      TEST_CHECK(domainRepoMap.find(domain)->second == key);

      OpenDDS::DCPS::TransportConfig_rch tconf =
        TransportRegistry::instance()->domain_default_config(domain);
      TEST_CHECK(!tconf.is_nil());
      OpenDDS::DCPS::TransportConfig_rch myconf =
        TransportRegistry::instance()->get_config("myconfig");
      TEST_CHECK(tconf == myconf);

      OpenDDS::DCPS::Discovery_rch discovery = TheServiceParticipant->get_discovery(domain);
      TEST_CHECK(discovery);
      OpenDDS::RTPS::RtpsDiscovery_rch rd =
        dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(discovery);
      TEST_CHECK(!rd.is_nil());
      TEST_CHECK(rd->resend_period().value().sec() == 29);
      TEST_CHECK(rd->pb() == 7399);
      TEST_CHECK(rd->dg() == 249);
      TEST_CHECK(rd->pg() == 2);
      TEST_CHECK(rd->d0() == 1);
      TEST_CHECK(rd->d1() == 9);
      TEST_CHECK(rd->dx() == 15);
      TEST_CHECK(rd->spdp_send_addrs().size() == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.1.1.1:10001")) == 1);
    }

    {
      DDS::DomainId_t domain = 98;
      OpenDDS::DCPS::Discovery::RepoKey key = "MultiSendAddr";
      const OpenDDS::DCPS::Service_Participant::DomainRepoMap& domainRepoMap =
        TheServiceParticipant->domainRepoMap();
      TEST_CHECK(domainRepoMap.find(domain) != domainRepoMap.end());
      TEST_CHECK(domainRepoMap.find(domain)->second == key);

      OpenDDS::DCPS::TransportConfig_rch tconf =
        TransportRegistry::instance()->domain_default_config(domain);
      TEST_CHECK(!tconf.is_nil());

      OpenDDS::DCPS::Discovery_rch discovery = TheServiceParticipant->get_discovery(domain);
      TEST_CHECK(discovery);
      OpenDDS::RTPS::RtpsDiscovery_rch rd =
        dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(discovery);
      TEST_CHECK(!rd.is_nil());
      TEST_CHECK(rd->spdp_send_addrs().size() == 10);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.1.1.1:10001")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.2.2.2:10002")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.3.3.3:10003")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.4.4.4:10004")) == 1);

      // Specifying 10.5.5.5:10005:2 gives us two ports, with default interval of 2
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.5.5.5:10005")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.5.5.5:10007")) == 1);

      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.6.6.6:10006")) == 1);

      // Specifying 10.7.7.7:10007:3:3 gives three ports, with interval of three
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.7.7.7:10007")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.7.7.7:10010")) == 1);
      TEST_CHECK(rd->spdp_send_addrs().count(NetworkAddress("10.7.7.7:10013")) == 1);
    }
    {
      DDS::DomainId_t domain = 97;
      OpenDDS::DCPS::Discovery_rch discovery = TheServiceParticipant->get_discovery(domain);
      TEST_CHECK(discovery);
      TEST_CHECK(discovery->key() == "MyDefaultDiscovery");
    }
#endif

    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in ConfigFile.cpp:");
    return 1;
  }
  return 0;
}
