#include "DcpsInfo_pch.h"
#include "DCPSInfo_i.h"
#include "FederatorConfig.h"
#include "FederatorManagerImpl.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

//If we need BIT support, pull in TCP so that static builds will have it.
#if !defined(DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif

#include "tao/ORB_Core.h"
#include "tao/IORTable/IORTable.h"

#include <orbsvcs/Shutdown_Utilities.h>

#ifdef ACE_AS_STATIC_LIBS
#include <tao/Version.h>
#if TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION == 4
// no ImR_Client, and we are already including it statically since we are
// statically linking to TAO_PortableServer
#else
#include "tao/ImR_Client/ImR_Client.h"
#endif
#endif

#include "ace/Get_Opt.h"
#include "ace/Arg_Shifter.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"
#include "ace/Event_Handler.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

class InfoRepo : public ACE_Event_Handler {
public:
  struct InitError
  {
    InitError (const char* msg)
      : msg_(msg) {};
    std::string msg_;
  };

  InfoRepo (int argc, ACE_TCHAR *argv[]) throw (InitError);
  ~InfoRepo (void);
  bool run (void);

  /// Callback used by the INT signal handler to schedule a shutdown.
  void shutdown (void);
  
  /// Handler for the reactor to dispatch finalization activity to.
  virtual int handle_exception( ACE_HANDLE fd = ACE_INVALID_HANDLE);

private:
  bool init (int argc, ACE_TCHAR *argv[]) throw (InitError);
  void usage (const ACE_TCHAR * cmd);
  void parse_args (int argc, ACE_TCHAR *argv[]);

  /// Actual finalization of service resources.
  void finalize();

  CORBA::ORB_var orb_;
  PortableServer::POA_var root_poa_;
  PortableServer::POA_var info_poa_;
  PortableServer::POAManager_var poa_manager_;

  ACE_TString ior_file_;
  ACE_TString listen_address_str_;
  int listen_address_given_;
  bool use_bits_;
  bool resurrect_;

  /// Flag to indicate that finalization has already occurred.
  bool finalized_;

  /// Repository Federation behaviors
  OpenDDS::Federator::ManagerImpl federator_;
  OpenDDS::Federator::Config      federatorConfig_;
};

InfoRepo::InfoRepo (int argc, ACE_TCHAR *argv[]) throw (InfoRepo::InitError)
  : ior_file_ (ACE_TEXT("repo.ior"))
    , listen_address_given_ (0)
    , use_bits_ (true)
    , resurrect_ (true)
    , finalized_( false)
    , federator_( this->federatorConfig_)
    , federatorConfig_( argc, argv)
{
  init (argc, argv);
}

InfoRepo::~InfoRepo (void)
{
  if( this->finalized_ == false) {
    this->finalize();
  }
  orb_->destroy ();
}

bool
InfoRepo::run (void)
{
  orb_->run ();

  return true;
}

void
InfoRepo::finalize()
{
  federator_.finalize();
  TheTransportFactory->release();
  TheServiceParticipant->shutdown ();
  this->finalized_ = true;
}

int
InfoRepo::handle_exception( ACE_HANDLE /* fd */)
{
  if( this->finalized_ == false) {
    this->finalize();
  }
  orb_->shutdown (0);
  return 0;
}

void
InfoRepo::shutdown (void)
{
  this->orb_->orb_core()->reactor()->notify( this);
}

void
InfoRepo::usage (const ACE_TCHAR * cmd)
{
  // NOTE: The federation arguments are parsed early by the
  //       FederationConfig object.
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("Usage:\n")
              ACE_TEXT ("  %s\n")
              ACE_TEXT ("    -a <address> listening address for Built-In Topics\n")
              ACE_TEXT ("    -o <file> write ior to file\n")
              ACE_TEXT ("    -NOBITS disable the Built-In Topics\n")
              ACE_TEXT ("    -z turn on verbose Transport logging\n")
              ACE_TEXT ("    -r Resurrect from persistent file\n")
              ACE_TEXT ("    -FederatorConfig <file> configure federation from <file>\n")
              ACE_TEXT ("    -FederationId <number> value for this repository\n")
              ACE_TEXT ("    -FederateWith <ior> federate initially with object at <ior>\n")
              ACE_TEXT ("    -?\n")
              ACE_TEXT ("\n"),
              cmd));
}

void
InfoRepo::parse_args (int argc,
                      ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ( (current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0)
        {
          listen_address_str_ = current_arg;
          listen_address_given_ = 1;
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter
                (ACE_TEXT("-r"))) != 0)
        {
          int p = ACE_OS::atoi (current_arg);
          resurrect_ = true;

          if (p == 0) {
            resurrect_ = false;
          }
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0)
        {
          ior_file_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-NOBITS")) == 0)
        {
          use_bits_ = false;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
        {
          TURN_ON_VERBOSE_DEBUG;
          arg_shifter.consume_arg();
        }

      // The '-?' option
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
        {
          this->usage (argv[0]);
          ACE_OS::exit (0);

        }
      // Anything else we just skip

      else
        {
          arg_shifter.ignore_arg();
        }
    }
}

bool
InfoRepo::init (int argc, ACE_TCHAR *argv[]) throw (InfoRepo::InitError)
{
  ACE_Argv_Type_Converter cvt(
                            this->federatorConfig_.argc(),
                            this->federatorConfig_.argv()
                          );

  orb_ = CORBA::ORB_init (cvt.get_argc(), cvt.get_ASCII_argv(), "");
  PortableServer::ServantBase_var info(
    new TAO_DDS_DCPSInfo_i(
      orb_.in(),
      resurrect_,
      this->federatorConfig_.federationId()
    )
  );
  TAO_DDS_DCPSInfo_i* info_servant = dynamic_cast<TAO_DDS_DCPSInfo_i*>(info.in());

  // Install the DCPSInfo_i into the Federator::Manager.
  this->federator_.info() = info_servant;

  CORBA::Object_var obj =
    orb_->resolve_initial_references ("RootPOA");
  root_poa_ = PortableServer::POA::_narrow (obj.in ());

  poa_manager_ =
    root_poa_->the_POAManager ();

  // Use persistent and user id POA policies so the Info Repo's
  // object references are consistent.
  CORBA::PolicyList policies (2);
  policies.length (2);
  policies[0] =
    root_poa_->create_id_assignment_policy (PortableServer::USER_ID);
  policies[1] =
    root_poa_->create_lifespan_policy (PortableServer::PERSISTENT);
  info_poa_ = root_poa_->create_POA ("InfoRepo",
                                     poa_manager_.in (),
                                     policies);

  // Creation of the new POAs over, so destroy the Policy_ptr's.
  for (CORBA::ULong i = 0; i < policies.length (); ++i) {
    policies[i]->destroy ();
  }

  PortableServer::ObjectId_var oid =
    PortableServer::string_to_ObjectId ("InfoRepo");
  info_poa_->activate_object_with_id (oid.in (),
                                      info.in());
  obj = info_poa_->id_to_reference(oid.in());
  // the object is created locally, so it is safe to do an 
  // _unchecked_narrow, this was needed to prevent an exception
  // when dealing with ImR-ified objects
  OpenDDS::DCPS::DCPSInfo_var info_repo
    = OpenDDS::DCPS::DCPSInfo::_unchecked_narrow (obj.in ());

  CORBA::String_var objref_str =
    orb_->object_to_string (info_repo.in ());

  TheServiceParticipant->set_ORB(orb_.in());

  // Initialize the DomainParticipantFactory
  ::DDS::DomainParticipantFactory_var dpf
      = TheParticipantFactoryWithArgs(argc, argv);

  // We need parse the command line options for DCPSInfoRepo after parsing DCPS specific
  // command line options.

  // Check the non-ORB arguments.
  this->parse_args(
    this->federatorConfig_.argc(),
    this->federatorConfig_.argv()
  );

  // Activate the POA manager before initialize built-in-topics
  // so invocations can be processed.
  poa_manager_->activate ();

  if (use_bits_)
    {
      if (0 != info_servant->init_transport(listen_address_given_, listen_address_str_.c_str()))
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: Failed to initialize the transport!\n")),
                           false);
        }
    }
  else
    {
      TheServiceParticipant->set_BIT(false);
    }

  // This needs to be done after initialization since we create the reference
  // to ourselves in the service here.
  TheServiceParticipant->set_repo_ior(
    ACE_TEXT_CHAR_TO_TCHAR(objref_str.in()),
    OpenDDS::DCPS::Service_Participant::DEFAULT_REPO
  );

  // Initialize persistence _after_ initializing the participant factory
  // and intializing the transport.
  if( false == info_servant->init_persistence()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
               ACE_TEXT("Unable to initialize persistence.\n")));
    throw InitError ("Unable to initialize persistence.");
  }

  // Fire up the federator.
  OpenDDS::Federator::Manager_var federator;
  CORBA::String_var               federator_ior;
  if( federator_.id() != ::OpenDDS::Federator::NIL_REPOSITORY) {
    oid = PortableServer::string_to_ObjectId ("Federator");
    info_poa_->activate_object_with_id (oid.in (),
                                        &federator_);
    obj = info_poa_->id_to_reference(oid.in());
    federator = OpenDDS::Federator::Manager::_narrow (obj.in ());

    federator_ior = orb_->object_to_string (federator.in ());

    // Add a local repository reference that can be returned via a
    // remote call to a peer.
    this->federator_.localRepo( info_repo.in());

    // It should be safe to initialize the federation mechanism at this
    // point.  What we really needed to wait for is the initialization of
    // the service components - like the DomainParticipantFactory and the
    // repository bindings.
    // N.B. This is done *before* being added to the IOR table to avoid any
    //      races with an eager client.
    this->federator_.orb( orb_.in());

    //
    // Add the federator to the info_servant update manager as an
    // additional updater interface to be called.
    // N.B. This needs to be done *after* the call to load_domains()
    //      since that is where the update manager is initialized in the
    //      info startup sequencing.
    info_servant->add( &this->federator_);
  }

  // Grab the IOR table.
  CORBA::Object_var table_object =
    orb_->resolve_initial_references ("IORTable");

  IORTable::Table_var adapter =
    IORTable::Table::_narrow (table_object.in ());
  if (CORBA::is_nil (adapter.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT("Nil IORTable\n")));

  } else {
    adapter->bind (::OpenDDS::Federator::REPOSITORY_IORTABLE_KEY, objref_str.in ());

    if( federator_.id() != ::OpenDDS::Federator::NIL_REPOSITORY) {
      // Bind to '/Federator'
      adapter->bind (::OpenDDS::Federator::FEDERATOR_IORTABLE_KEY,  federator_ior.in ());

      // Bind to '/Federator/1382379631'
      std::stringstream buffer( ::OpenDDS::Federator::FEDERATOR_IORTABLE_KEY);
      buffer << "/" << std::dec << this->federatorConfig_.federationDomain();
      adapter->bind (buffer.str().c_str(),  federator_ior.in ());
    }
  }

  FILE *output_file= ACE_OS::fopen (ior_file_.c_str (), ACE_TEXT("w"));
  if (output_file == 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Unable to open IOR file: %s\n")
                 , ior_file_.c_str()));
      throw InitError ("Unable to open IOR file.");
  }

  ACE_OS::fprintf (output_file, "%s", objref_str.in ());
  ACE_OS::fclose (output_file);

  // Initial federation join if specified on command line.
  if( (this->federator_.id() > 0)
   && (false == this->federatorConfig_.federateIor().empty())
    ) {
    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) INFO: DCPSInfoRepo::init() - ")
        ACE_TEXT("joining federation with repository %s\n"),
        this->federatorConfig_.federateIor().c_str()
      ));
    }

    CORBA::Object_var obj
      = this->orb_->string_to_object( this->federatorConfig_.federateIor().c_str());
    if( CORBA::is_nil( obj.in())) {
      ACE_ERROR(( LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: could not resolve %s for initial federation.\n"),
        this->federatorConfig_.federateIor().c_str()
      ));
      throw InitError( "Unable to resolve IOR for initial federation.");
    }

    OpenDDS::Federator::Manager_var peer
      = OpenDDS::Federator::Manager::_narrow( obj.in() );
    if( CORBA::is_nil( peer.in())) {
      ACE_ERROR(( LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: could not narrow %s.\n"),
        this->federatorConfig_.federateIor().c_str()
      ));
      throw InitError( "Unable to narrow peer for initial federation.");
    }

    // Actually join.
    peer->join_federation(
      federator.in(),
      this->federatorConfig_.federationDomain()
    );
  }

  return true;
}

class InfoRepo_Shutdown : public Shutdown_Functor
{
public:
  InfoRepo_Shutdown(InfoRepo& ir);

  void operator() (int which_signal);
private:
  InfoRepo& ir_;
};

InfoRepo_Shutdown::InfoRepo_Shutdown (InfoRepo &ir)
  : ir_(ir)
{
}

void
InfoRepo_Shutdown::operator() (int which_signal)
{
  ACE_DEBUG ((LM_DEBUG,
              "InfoRepo_Shutdown: shutting down on signal %d\n",
              which_signal));
  ir_.shutdown ();
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  try
    {
      InfoRepo infoRepo (argc, argv);

      InfoRepo_Shutdown ir_shutdown (infoRepo);
      Service_Shutdown service_shutdown(ir_shutdown);

      infoRepo.run ();
    }
  catch (InfoRepo::InitError& ex)
    {
      std::cerr << "Unexpected initialization Error: "
                << ex.msg_ << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: ::DDS DCPS Info Repo caught exception");

      return -1;
    }

  return 0;
}
