#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Registered_Data_Types.h>
#include <dds/DCPS/XTypes/DynamicTypeSupport.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <ace/DLL.h>

#include <cstring>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
#ifdef ACE_AS_STATIC_LIBS
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);
#else
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  ACE_DLL ts_plugin;
  if (ts_plugin.open(ACE_TEXT("ConsolidatedMessengerIdl"))) {
    ACE_ERROR((LM_ERROR, "ERROR: Failed to open the type support library\n"));
    return 1;
  }

  const char* const type_name = "Messenger::Message";
  ACE_DEBUG((LM_DEBUG, "Getting TypeSupport for %C\n", type_name));
  DDS::TypeSupport_var ts = Registered_Data_Types->lookup(0, type_name);
  if (!ts) {
    ACE_ERROR((LM_ERROR, "ERROR: TypeSupport wasn't registered!\n"));
    return 1;
  }

  CORBA::String_var ts_name = ts->get_type_name();
  if (std::strcmp(ts_name.in(), type_name)) {
    ACE_ERROR((LM_ERROR, "ERROR: got name from type support: \"%C\", expected \"%C\"\n",
      ts_name.in(), type_name));
  }

#  ifndef OPENDDS_SAFETY_PROFILE
  // Set default to RTPS, create DynamicDataWriter from dlopened TypeSupport
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
  OpenDDS::DCPS::TransportConfig_rch transport_config =
    TheTransportRegistry->create_config("default_rtps_transport_config");
  OpenDDS::DCPS::TransportInst_rch transport_inst =
    TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
  transport_config->instances_.push_back(transport_inst);
  TheTransportRegistry->global_config(transport_config);

  DDS::DomainParticipant_var dp = dpf->create_participant(
    12, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!dp) {
    ACE_ERROR((LM_ERROR, "ERROR: create_participant failed!\n"));
    return 1;
  }

  DDS::DynamicType_var type = ts->get_type();
  DDS::DynamicTypeSupport_var dts = new DDS::DynamicTypeSupport(type);
  DDS::ReturnCode_t rc = dts->register_type(dp, "");
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: register_type failed: %C\n", OpenDDS::DCPS::retcode_to_string(rc)));
    return 1;
  }

  DDS::Topic_var topic = dp->create_topic(
    "Topic", ts_name.in(), TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!topic) {
    ACE_ERROR((LM_ERROR, "ERROR: create_topic failed!\n"));
    return 1;
  }

  DDS::Publisher_var pub = dp->create_publisher(
    PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed!\n"));
    return 1;
  }

  DDS::DataWriter_var dw = pub->create_datawriter(
    topic, DATAWRITER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datawriter failed!\n"));
    return 1;
  }

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
#  endif // OPENDDS_SAFETY_PROFILE
#endif // ACE_AS_STATIC_LIBS

  return 0;
}
