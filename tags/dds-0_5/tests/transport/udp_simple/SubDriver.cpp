#include "SubDriver.h"
#include "TestException.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpFactory.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpTransport.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration_rch.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
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
  DBG_SUB_ENTRY("SubDriver","run",1);

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
             "Use TheTransportFactory to register a new SimpleUdpFactory "
             "object with the SIMPLE_UDP type_id (%d).\n", SIMPLE_UDP));

  // SIMPLE_UDP is a "type_id".  The register_type() method tells
  // TheTransportFactory which TransportImplFactory object it should
  // delegate to when it is asked to create() SIMPLE_UDP TransportImpl
  // objects.  For this test, this is the only type we register with
  // TheTransportFactory.
  TheTransportFactory->register_type(SIMPLE_UDP,
                                     new TAO::DCPS::SimpleUdpFactory());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use TheTransportFactory to create a TransportImpl object "
             "of type_id SIMPLE_UDP with the ALL_TRAFFIC transport_id (%d).\n",
             ALL_TRAFFIC));

  // Now we can ask TheTransportFactory to create a TransportImpl object
  // using the SIMPLE_UDP type_id.  We also supply an identifier for this
  // particular TransportImpl object that will be created.  This is known
  // as the "impl_id", or "the TransportImpl's instance id".  The point is
  // that we assign the impl_id, and TheTransportFactory caches a reference
  // to the newly created TransportImpl object using the impl_id (ALL_TRAFFIC
  // in our case) as a key to the cache map.  Other parts of this client
  // application code will be able use the obtain() method on
  // TheTransportFactory, provide the impl_id (ALL_TRAFFIC in our case), and
  // a reference to the cached TransportImpl will be returned.
  TAO::DCPS::TransportImpl_rch transport_impl =
                          TheTransportFactory->create(ALL_TRAFFIC,SIMPLE_UDP);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create a new SimpleUdpConfiguration object.\n"));

  TAO::DCPS::SimpleUdpConfiguration_rch config =
                                      new TAO::DCPS::SimpleUdpConfiguration();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the config->local_address_ to our (local) sub_addr_.\n"));

  config->local_address_ = this->sub_addr_;

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
  DBG_SUB_ENTRY("SubDriver","run",2);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the 'publications' (array of AssociationData).\n"));

  // Set up the publications.
  TAO::DCPS::AssociationData publications[1];
  publications[0].remote_id_                = this->pub_id_;
  publications[0].remote_data_.transport_id = 2; // TBD later - wrong

  TAO::DCPS::NetworkAddress network_order_address(this->pub_addr_);

  publications[0].remote_data_.data =
         TAO::DCPS::TransportInterfaceBLOB
                                   (sizeof(TAO::DCPS::NetworkAddress),
                                    sizeof(TAO::DCPS::NetworkAddress),
                                    (CORBA::Octet*)(&network_order_address));

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
  std::string pub_addr_str(arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  this->pub_id_ = ACE_OS::atoi(pub_id_str.c_str());

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = pub_addr_str.find_first_of(':')) == std::string::npos) {
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
  this->pub_addr_ = ACE_INET_Addr(pub_addr_str.c_str());

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
  std::string sub_addr_str(arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  this->sub_id_ = ACE_OS::atoi(sub_id_str.c_str());

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->sub_addr_ = ACE_INET_Addr(sub_addr_str.c_str());

  return 0;
}
