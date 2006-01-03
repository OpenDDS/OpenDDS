#include "DcpsInfo_pch.h"
#include "DCPSInfo_i.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

#include "tao/ORB_Core.h"

#include "ace/Get_Opt.h"
#include "ace/Arg_Shifter.h"


static const char * ior_file = "repo.ior";
static const char * domain_file = "domain_ids";
static const char * listen_address_str = "localhost:2839"; // = 0xB17
static int listen_address_given = 0;
static bool use_bits = true;

void
usage (const ACE_TCHAR * cmd)
{
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("Usage:\n")
              ACE_TEXT ("  %s\n")
              ACE_TEXT ("    -a <address> listening address for Built-In Topics\n")
              ACE_TEXT ("    -o <file> write ior to file\n")
              ACE_TEXT ("    -d <file> load domain ids from file\n")
              ACE_TEXT ("    -NOBITS disable the Built-In Topics\n")
              ACE_TEXT ("    -z turn on verbose Transport logging\n")
              ACE_TEXT ("    -?\n")
              ACE_TEXT ("\n"),
              cmd));
}

void
parse_args (int argc,
            ACE_TCHAR *argv[]
            ACE_ENV_ARG_DECL)
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ( (current_arg = arg_shifter.get_the_parameter("-a")) != 0)
        {
          ::listen_address_str = current_arg;
          listen_address_given = 1;
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-d")) != 0) 
        {
          ::domain_file = current_arg;
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-o")) != 0) 
        {
          ::ior_file = current_arg;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp("-NOBITS") == 0) 
        {
          ::use_bits = false;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
        {
          TURN_ON_VERBOSE_DEBUG;
          arg_shifter.consume_arg();
        }

      // The '-?' option
      else if (arg_shifter.cur_arg_strncasecmp("-?") == 0)
        {
          ::usage (argv[0]);
          ACE_OS::exit (0);

        }
      // Anything else we just skip
      else
        {
          arg_shifter.ignore_arg();
        }
    }
}


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
 
  ACE_DEBUG((LM_DEBUG,"(%P|%t) %T Repo main\n")); //REMOVE
 
  ACE_DECLARE_NEW_CORBA_ENV;
  ACE_TRY
    {
      // The usual server side boilerplate code.

      CORBA::ORB_var orb = CORBA::ORB_init (argc,
                                            argv,
                                            ""
                                            ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      CORBA::Object_var obj =
        orb->resolve_initial_references ("RootPOA"
                                         ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow (obj.in ()
                                      ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      PortableServer::POAManager_var poa_manager =
        root_poa->the_POAManager (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;

      poa_manager->activate (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;


      // Check the non-ORB arguments.
      ::parse_args (argc,
                    argv
                    ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      TAO_DDS_DCPSInfo_i info;

      PortableServer::ObjectId_var oid = root_poa->activate_object(&info);
      obj = root_poa->id_to_reference(oid.in());
      TAO::DCPS::DCPSInfo_var info_repo = TAO::DCPS::DCPSInfo::_narrow(
                      obj.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      CORBA::String_var str =
        orb->object_to_string (info_repo.in ()
                               ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      TheServiceParticipant->set_ORB(orb.in());
      TheServiceParticipant->set_repo_ior(str.in());

      // Initialize the DomainParticipantFactory
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_TRY_CHECK;

      if (::use_bits) 
        {
          ACE_INET_Addr address (::listen_address_str);
          if (0 != info.init_transport(listen_address_given, address))
            {
              ACE_ERROR_RETURN((LM_ERROR, 
                                ACE_TEXT("ERROR: Failed to initialize the transport!\n")),
                               -1);
            }
        }

      // Load the domains _after_ initializing the participant factory and initializing
      // the transport
      if (0 >= info.load_domains(::domain_file, ::use_bits))
        {
          //ACE_ERROR_RETURN((LM_ERROR, "ERROR: Failed to load any domains!\n"), -1);
        }

      FILE * file = ACE_OS::fopen (::ior_file, "w");
      ACE_OS::fprintf (file, "%s", str.in ());
      ACE_OS::fclose (file);


      orb->run (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;

      TheServiceParticipant->shutdown (); 

      orb->destroy (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "ERROR: ::DDS DCPS Info Repo caught exception");

      return -1;
    }
  ACE_ENDTRY;

  return 0;
}
