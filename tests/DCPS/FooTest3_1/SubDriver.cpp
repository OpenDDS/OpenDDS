#include "SubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "SimpleSubscriber.h"
#include "tests/DCPS/common/TestSupport.h"

#include <ace/Arg_Shifter.h>
#include <ace/OS.h>
#include <string>

SubDriver::SubDriver()
: pub_id_fname_ ("pub_id.txt"),
  sub_id_ (0),
  num_writes_ (0),
  receive_delay_msec_ (0),
  pub_driver_ior_ ("file://pubdriver.ior")
{
}


SubDriver::~SubDriver()
{
}


void
SubDriver::run(int& argc, char* argv[])
{
  parse_args(argc, argv);
  init(argc, argv);
  run();
}


void
SubDriver::parse_args(int& argc, char* argv[])
{
  // Command-line arguments:
  //
  // -p <pub_id_fname:pub_host:pub_port>
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
    else if ((current_arg = arg_shifter.get_the_parameter("-n")) != 0) 
    {
      num_writes_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
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
    else if ((current_arg = arg_shifter.get_the_parameter("-v")) != 0) 
    {
      pub_driver_ior_ = current_arg;
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter("-l")) != 0) 
    {
      receive_delay_msec_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
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
SubDriver::init(int& argc, char* argv[])
{
  // initialize the orb
  orb_ = CORBA::ORB_init (argc, 
                          argv, 
                          "TAO_DDS_DCPS" 
                          ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TAO::DCPS::TransportImpl_rch transport_impl 
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC, "SimpleTcp", TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch config 
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleTcp");

  TAO::DCPS::SimpleTcpConfiguration* tcp_config 
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (config.in ());

  tcp_config->local_address_ = this->sub_addr_;

  if (transport_impl->configure(config.in ()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }
  // And we are done with the init().
}


void
SubDriver::run()
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) SubDriver::run, ")
              ACE_TEXT(" Wait for %s. \n"), 
              pub_id_fname_.c_str ()));
    
  PublicationIds ids;

  // Wait for the publication id file created by the publisher.
  while (1)
  {
    FILE* fp 
      = ACE_OS::fopen (pub_id_fname_.c_str (), ACE_LIB_TEXT("r"));
    if (fp == 0)
    {
      ACE_OS::sleep (1);
    }
    else 
    { 
      ::TAO::DCPS::PublicationId pub_id = 0;
      while (fscanf (fp, "%d\n", &pub_id) != EOF)
      {
        ids.push_back (pub_id);
        ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) SubDriver::run, ")
              ACE_TEXT(" Got from %s: pub_id=%d. \n"), 
              pub_id_fname_.c_str (),
              pub_id));
      }
      ACE_OS::fclose (fp);
      break;
    }
  }

  CORBA::Object_var object =
    orb_->string_to_object (pub_driver_ior_.c_str () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  pub_driver_ = ::Test::TestPubDriver::_narrow (object.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (!CORBA::is_nil (pub_driver_.in ()));

  size_t num_publications = ids.size ();

  // Set up the publications.
  TAO::DCPS::AssociationData* publications 
    = new TAO::DCPS::AssociationData[num_publications];
  

  for (size_t i = 0; i < num_publications; i ++)
  {
    publications[i].remote_id_                = ids[i];
    publications[i].remote_data_.transport_id = ALL_TRAFFIC; // TBD later - wrong

    TAO::DCPS::NetworkAddress network_order_address(this->pub_addr_);
    publications[i].remote_data_.data 
      = TAO::DCPS::TransportInterfaceBLOB
                  (sizeof(TAO::DCPS::NetworkAddress),
                   sizeof(TAO::DCPS::NetworkAddress),
                   (CORBA::Octet*)(&network_order_address));
  }

  this->subscriber_.init(ALL_TRAFFIC,
                         this->sub_id_,
                         num_publications, 
                         publications, 
                         receive_delay_msec_);

  delete [] publications;

  while (this->subscriber_.received_test_message() != num_writes_)
    {
      ACE_OS::sleep(1);
    }

  pub_driver_->shutdown ();

  // Sleep before release transport so the connection will not go away. 
  // This would avoid the problem of publisher sendv failure due to lost 
  // connection during the shutdown period.
  ACE_OS::sleep (5);
  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
}

int
SubDriver::parse_pub_arg(const std::string& arg)
{
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

  this->pub_id_fname_ = pub_id_str.c_str();
  this->pub_addr_ = ACE_INET_Addr(pub_addr_str.c_str());

  return 0;
}

int
SubDriver::parse_sub_arg(const std::string& arg)
{
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value  (%s) - "
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


