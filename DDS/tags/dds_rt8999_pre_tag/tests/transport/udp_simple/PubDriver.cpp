#include "PubDriver.h"
#include "TestException.h"
// Include the SimpleUdp.h to make sure Initializer is created before the Service 
// Configurator open service configure file.
#include "dds/DCPS/transport/simpleUDP/SimpleUdp.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "SimplePublisher.h"
#include <ace/Arg_Shifter.h>

#include "dds/DCPS/transport/framework/EntryExit.h"


PubDriver::PubDriver()
{
  DBG_ENTRY("PubDriver","PubDriver");
}


PubDriver::~PubDriver()
{
  DBG_ENTRY("PubDriver","~PubDriver");
}


void
PubDriver::run(int& argc, char* argv[])
{
  DBG_SUB_ENTRY("PubDriver","run",1);

  // Need call the ORB_init to dynamically load the SimpleUdp library via
  // service configurator.
  // initialize the orb
  CORBA::ORB_var orb = CORBA::ORB_init (argc, 
                                        argv, 
                                        "TAO_DDS_DCPS" 
                                        ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  this->parse_args(argc, argv);
  this->init();
  this->run();
}


void
PubDriver::parse_args(int& argc, char* argv[])
{
  DBG_ENTRY("PubDriver","parse_args");

  // Command-line arguments:
  //
  // -p <pub_id:pub_port>
  // -s <sub_id:sub_host:sub_port>
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
PubDriver::init()
{
  DBG_ENTRY("PubDriver","init");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use TheTransportFactory to create a TransportImpl object "
             "of SimpleUdp with the ALL_TRAFFIC transport_id (%d).\n",
             ALL_TRAFFIC));

  TAO::DCPS::TransportImpl_rch transport_impl 
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC, 
                                                  "SimpleUdp",
                                                  TAO::DCPS::DONT_AUTO_CONFIG);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Get the existing or create a new SimpleUdpConfiguration object.\n"));

  TAO::DCPS::TransportConfiguration_rch config 
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleUdp");

  TAO::DCPS::SimpleUdpConfiguration* udp_config 
    = static_cast <TAO::DCPS::SimpleUdpConfiguration*> (config.in ());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the config->local_address_ to our (local) pub_addr_.\n"));

  udp_config->local_address_ = this->pub_addr_;


  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Configure the (ALL_TRAFFIC) TransportImpl object.\n"));

  // Pass the configuration object to the transport_impl's configure()
  // method as an "in" argument.  Don't change the configuration object
  // after this has been done since it will cause unknown results.
  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The TransportImpl object has been successfully configured.\n"));
}


void
PubDriver::run()
{
  DBG_SUB_ENTRY("PubDriver","run",2);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the 'subscriptions' (array of AssociationData).\n"));

  // Set up the subscriptions.
  TAO::DCPS::AssociationData subscriptions[1];
  subscriptions[0].remote_id_                = this->sub_id_;
  subscriptions[0].remote_data_.transport_id = 2;  // TBD - not right

  TAO::DCPS::NetworkAddress network_order_address(this->sub_addr_);

  subscriptions[0].remote_data_.data = TAO::DCPS::TransportInterfaceBLOB
                       (sizeof(TAO::DCPS::NetworkAddress),
                        sizeof(TAO::DCPS::NetworkAddress),
                        (CORBA::Octet*)(&network_order_address));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Initialize our SimplePublisher object.\n"));

  this->publisher_.init(ALL_TRAFFIC,
                        this->pub_id_,
                        1,               /* size of subscriptions array */
                        subscriptions);

  // Wait for a fully association establishment and then start sending samples.
  ACE_OS::sleep (2);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Run our SimplePublisher object.\n"));

  this->publisher_.run();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Ask the SimplePublisher object if it is done running. "
             "If not, sleep for 1 second, and ask again.\n"));

  while (this->publisher_.delivered_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The SimplePublisher object is done running.  "
             "Release TheTransportFactory - causing all TransportImpl "
             "objects to be shutdown().\n"));

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "TheTransportFactory has finished releasing.\n"));
}


int
PubDriver::parse_pub_arg(const std::string& arg)
{
  DBG_ENTRY("PubDriver","parse_pub_arg");

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
  this->pub_addr_ = ACE_INET_Addr(pub_addr_str.c_str());

  return 0;
}


int
PubDriver::parse_sub_arg(const std::string& arg)
{
  DBG_ENTRY("PubDriver","parse_sub_arg");

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

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = sub_addr_str.find_first_of(':')) == std::string::npos) {
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
  this->sub_addr_ = ACE_INET_Addr(sub_addr_str.c_str());

  return 0;
}
