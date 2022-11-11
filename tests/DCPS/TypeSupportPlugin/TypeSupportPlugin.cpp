#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Registered_Data_Types.h>

#include <ace/DLL.h>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
#ifndef ACE_AS_STATIC_LIBS
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  ACE_DLL ts_plugin;
  if (ts_plugin.open(ACE_TEXT("ConsolidatedMessengerIdl"))) {
    ACE_ERROR((LM_ERROR, "ERROR: Failed to open the type support library\n"));
    return 1;
  }

  const char* type_name = "Messenger::Message";
  printf("Getting TypeSupport for %s\n", type_name);
  DDS::TypeSupport_ptr ts = Registered_Data_Types->lookup(0, type_name);
  if (!ts) {
    ACE_ERROR((LM_ERROR, "ERROR: TypeSupport wasn't registered!\n"));
    return 1;
  }

  CORBA::String_var ts_name = ts->get_type_name();
  printf("Name according to type support is %s\n", ts_name.in());
#endif // ACE_AS_STATIC_LIBS

  return 0;
}
