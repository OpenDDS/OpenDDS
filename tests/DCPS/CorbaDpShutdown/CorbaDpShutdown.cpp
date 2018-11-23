// $Id: CorbaDpShutdown.cpp dczanella@gmail.com
#include "CorbaDpShutdown.h"


using namespace std;


CorbaDpShutdown::CorbaDpShutdown (CORBA::ORB_ptr orb)
  : orb_ (CORBA::ORB::_duplicate (orb))
{
}

void
CorbaDpShutdown::shutdown (void)
{
  this->orb_->shutdown (0);
}

void CorbaDpShutdown::start()
{
  try
    {
      dpf = TheParticipantFactory;

      OpenDDS::DCPS::TransportConfig_rch config =
        OpenDDS::DCPS::TransportRegistry::instance()->get_config("dds4ccm_rtps");
//      OpenDDS::DCPS::TransportConfig_rch config =
//    		  OpenDDS::DCPS::TransportRegistry::instance()->global_config();

      if (config.is_nil())
        {
          config =
            OpenDDS::DCPS::TransportRegistry::instance()->create_config("dds4ccm_rtps");
        }

      OpenDDS::DCPS::TransportInst_rch inst =
        OpenDDS::DCPS::TransportRegistry::instance()->get_inst("the_rtps_transport");

      if (inst.is_nil())
        {
          inst =
            OpenDDS::DCPS::TransportRegistry::instance()->create_inst("the_rtps_transport",
                                                                "rtps_udp");

          config->instances_.push_back(inst);

          OpenDDS::DCPS::TransportRegistry::instance()->domain_default_config(DOMAIN_ID,config);
        }


      OpenDDS::RTPS::RtpsDiscovery_rch disc =
        OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
//      disc->sedp_local_address("10.0.2.15");
//      disc->spdp_local_address("10.0.2.15");
//      disc->sedp_multicast(true);
//      disc->default_multicast_group("224.0.0.1");
//      disc->multicast_interface("eth0");




      // The recommended value for the resend period is 2 seconds for
      // the current implementation of OpenDDS.
      disc->resend_period(ACE_Time_Value(2));

      TheServiceParticipant->add_discovery(disc);
      TheServiceParticipant->set_repo_domain(DOMAIN_ID, disc->key());


      inst->dump_to_str();

      participant =
        dpf->create_participant(DOMAIN_ID,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return;
      }
      else
      {
        ACE_DEBUG ((LM_DEBUG, "Created participant with instance handle %d\n",
                    participant->get_instance_handle ()));
      }
      try
      {
        OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config, participant.in());
      }
      catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
	ACE_DEBUG ((LM_DEBUG, "ERROR: TransportRegistry::bind_config() throws Transport::MiscProblem exception\n"));
      }
      catch (const OpenDDS::DCPS::Transport::NotFound &) {
	ACE_DEBUG ((LM_DEBUG, "ERROR: TransportRegistry::bind_config() throws Transport::NotFound exception\n"));
      }

  }
  catch (CORBA::Exception& e)
  {
    cerr << "dp: Exception caught in start method:" << endl
         << e << endl;
  }
}

void CorbaDpShutdown::stop()
{
      dpf = TheParticipantFactory;

      DDS::ReturnCode_t retcode6 = dpf->delete_participant(participant.in ());
      if (retcode6 != DDS::RETCODE_OK) {
	ACE_DEBUG ((LM_DEBUG, "ERROR: should be able to delete participant\n"));
      }

  ACE_DEBUG ((LM_DEBUG, "Shutting down the service participant\n"));
  TheServiceParticipant->shutdown ();
}
