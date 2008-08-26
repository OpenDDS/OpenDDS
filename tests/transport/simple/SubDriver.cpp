#include "SubDriver.h"
#include "TestException.h"

// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "SimpleSubscriber.h"
#include <ace/Arg_Shifter.h>
#include <ace/OS.h>

#include "dds/DCPS/transport/framework/EntryExit.h"


SubDriver::SubDriver()
{
  DBG_ENTRY("SubDriver","SubDriver");
}


SubDriver::~SubDriver()
{
  DBG_ENTRY("SubDriver","~SubDriver");
}


void
SubDriver::run(int& argc, char* argv[])
{
  DBG_ENTRY_LVL("SubDriver","run",6);
  CORBA::ORB_var orb = CORBA::ORB_init (argc,
                                        argv,
                                        "TAO_DDS_DCPS");

  TheServiceParticipant->set_ORB (orb.in());

  parse_args(argc, argv);
  init();
  run();
}


void
SubDriver::parse_args(int& argc, char* argv[])
{
  DBG_ENTRY("SubDriver","parse_args");

  // Command-line arguments:
  //
  // -p <pub_id:pub_host:pub_port>
  // -s <sub_id:sub_port>
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_p = false;
  bool got_s = false;

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter("-p"))) {
      if (got_p) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -p allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_pub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -p command-line arg.\n"));
        throw TestException();
      }

      got_p = true;
    }
    // A '-s' option
    else if ((current_arg = arg_shifter.get_the_parameter("-s"))) {
      if (got_s) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -s allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_sub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -s command-line arg.\n"));
        throw TestException();
      }

      got_s = true;
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp("-?") == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p pub_id:pub_host:pub_port -s sub_id:sub_host:sub_port\n",
                 argv[0]));

      arg_shifter.consume_arg();
      throw TestException();
    }
    // Anything else we just skip
    else {
      arg_shifter.ignore_arg();
    }
  }

  // Make sure we got the required arguments:
  if (!got_p) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -p command-line option not specified (required).\n"));
    throw TestException();
  }

  if (!got_s) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -s command-line option not specified (required).\n"));
    throw TestException();
  }
}


void
SubDriver::init()
{
  DBG_ENTRY("SubDriver","init");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use TheTransportFactory to create a TransportImpl object "
             "of SimpleTcp with the ALL_TRAFFIC transport_id (%d).\n",
             ALL_TRAFFIC));

  // Now we can ask TheTransportFactory to create a TransportImpl object
  // using the "SimpleTcp" factory.  We also supply an identifier for this
  // particular TransportImpl object that will be created.  This is known
  // as the "impl_id", or "the TransportImpl's instance id".  The point is
  // that we assign the impl_id, and TheTransportFactory caches a reference
  // to the newly created TransportImpl object using the impl_id (ALL_TRAFFIC
  // in our case) as a key to the cache map.  Other parts of this client
  // application code will be able use the obtain() method on
  // TheTransportFactory, provide the impl_id (ALL_TRAFFIC in our case), and
  // a reference to the cached TransportImpl will be returned.
  OpenDDS::DCPS::TransportImpl_rch transport_impl 
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC, 
                                                  "SimpleTcp",
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create a new SimpleTcpConfiguration object.\n"));

  // Get the existing or create a new SimpleTcpConfiguration object.  It just has one field
  // to set - the local_address_ field.  This is the address that will be
  // used to open an acceptor object to listen for passive connection
  // requests.  This is the TransportImpl object's (local) "endpoint" address.
  // See comments in the $TAO_ROOT/orbsvcs/tests/DDS/transport/simple/
  // PubDriver.cpp (in the PubDriver::init() method) that describes the
  // other configuration options available.
  OpenDDS::DCPS::TransportConfiguration_rch config 
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleTcp");

  OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config 
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (config.in ());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the config->local_address_ to our (local) sub_addr_.\n"));

  tcp_config->local_address_ = this->sub_addr_;
  tcp_config->local_address_str_ = this->sub_addr_str_;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Configure the (ALL_TRAFFIC) TransportImpl object.\n"));

  // Supply the config object to the TranportImpl object via its configure()
  // method.
  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }

  // And we are done with the init().
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The TransportImpl object has been successfully configured.\n"));
}


void
SubDriver::run()
{
  DBG_ENTRY_LVL("SubDriver","run",6);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the 'publications' (array of AssociationData).\n"));

  // Set up the publications.
  OpenDDS::DCPS::AssociationData publications[1];
  publications[0].remote_id_                = this->pub_id_;
  publications[0].remote_data_.transport_id = ALL_TRAFFIC; // TBD later - wrong

  OpenDDS::DCPS::NetworkAddress network_order_address(this->pub_addr_str_);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length ();

  publications[0].remote_data_.data
    = OpenDDS::DCPS::TransportInterfaceBLOB
    (len,
    len,
    (CORBA::Octet*)(cdr.buffer ()));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Initialize our SimpleSubscriber object.\n"));

  // Write a file so that test script knows we're ready
  FILE * file = ACE_OS::fopen ("subready.txt", "w");
  ACE_OS::fprintf (file, "Ready\n");
  ACE_OS::fclose (file);

  this->subscriber_.init(ALL_TRAFFIC,
                         this->sub_id_,
                         1,               /* size of publications array */
                         publications);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Ask the SimpleSubscriber object if it has received what, "
             "it expected.  If not, sleep for 1 second, and ask again.\n"));

  // Wait until we receive our expected message from the remote
  // publisher.  For this test, we should wait until we receive the
  // "Hello World!" message that we expect.  Then this program
  // can just shutdown.
  while (this->subscriber_.received_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The SimpleSubscriber object has received what it expected.  "
             "Release TheTransportFactory - causing all TransportImpl "
             "objects to be shutdown().\n"));

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "TheTransportFactory has finished releasing.\n"));
}


int
SubDriver::parse_pub_arg(const std::string& arg)
{
  DBG_ENTRY("SubDriver","parse_pub_arg");

  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the pub_id from left of ':' char, and remainder to right of ':'.
  std::string pub_id_str(arg,0,pos);
  this->pub_addr_str_ = std::string (arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  OpenDDS::DCPS::GuidConverter converter( 0, 1); // Federation == 0, Participant == 1
  converter.kind()   = OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY;
  converter.key()[2] = ACE_OS::atoi( pub_id_str.c_str());
  this->pub_id_ = converter;

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = this->pub_addr_str_.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "Missing second ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s) - "
               "Second ':' char immediately follows first ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s) - "
               "Second ':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->pub_addr_ = ACE_INET_Addr(this->pub_addr_str_.c_str());

  return 0;
}


int
SubDriver::parse_sub_arg(const std::string& arg)
{
  DBG_ENTRY("SubDriver","parse_sub_arg");

  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the sub_id from left of ':' char, and remainder to right of ':'.
  std::string sub_id_str(arg,0,pos);
  this->sub_addr_str_ = std::string (arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  OpenDDS::DCPS::GuidConverter converter( 0, 1); // Federation == 0, Participant == 2
  converter.kind()   = OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY;
  converter.key()[2] = ACE_OS::atoi( sub_id_str.c_str());
  this->sub_id_ = converter;

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->sub_addr_ = ACE_INET_Addr(this->sub_addr_str_.c_str());

  return 0;
}
