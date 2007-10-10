#include "PubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleMcastConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "SimplePublisher.h"
#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>

PubDriver::PubDriver()
  : num_msgs_(100)
{
}


PubDriver::~PubDriver()
{
}


void
PubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  // Need call the ORB_init to dynamically load the SimpleMcast library via
  // service configurator.
  // initialize the orb
  ACE_Argv_Type_Converter conv(argc, argv);
  CORBA::ORB_var orb = CORBA::ORB_init (conv.get_argc(),
                                        conv.get_ASCII_argv(),
                                        "TAO_DDS_DCPS");
  parse_args(argc, argv);
  init();
  run();
}


void
PubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  // -p <pub_id:pub_port>
  // -s <sub_id:sub_host:sub_port>
  // -n num_messages
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_n = false;
  bool got_p = false;
  bool got_s = false;

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-n' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n")))) {
      if (got_n) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -n allowed on command-line.\n"));
        throw TestException();
      }

      int value = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (value <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Value following -n option must be > 0.\n"));
        throw TestException();
      }

      this->num_msgs_ = value;

      got_n = true;
    }

    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-p")))) {
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
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-s")))) {
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
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p pub_id:pub_host:pub_port -s sub_id:if_addr\n",
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

  OpenDDS::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC,
                                                  ACE_TEXT("SimpleMcast"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);
  OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, ACE_TEXT("SimpleMcast"));

  OpenDDS::DCPS::SimpleMcastConfiguration* mcast_config
    = static_cast <OpenDDS::DCPS::SimpleMcastConfiguration*> (config.in ());

  //mcast_config->local_address_ = this->if_addr_;
  mcast_config->multicast_group_address_ = this->pub_addr_;

  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }
}


void
PubDriver::run()
{
  // Set up the subscriptions.
  OpenDDS::DCPS::AssociationData subscriptions[1];
  subscriptions[0].remote_id_                = this->sub_id_;
  subscriptions[0].remote_data_.transport_id = 3;  // TBD - not right

  OpenDDS::DCPS::NetworkAddress network_order_address(this->pub_addr_);

  subscriptions[0].remote_data_.data = OpenDDS::DCPS::TransportInterfaceBLOB
                       (sizeof(OpenDDS::DCPS::NetworkAddress),
                        sizeof(OpenDDS::DCPS::NetworkAddress),
                        (CORBA::Octet*)(&network_order_address));

  this->publisher_.init(ALL_TRAFFIC,
                        this->pub_id_,
                        1,               /* size of subscriptions array */
                        subscriptions);

  // Wait for a fully association establishment and then start sending samples.
  ACE_OS::sleep (2);

  this->publisher_.run(this->num_msgs_);

  while (this->publisher_.delivered_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
}


int
PubDriver::parse_pub_arg(const ACE_TString& arg)
{
  ACE_TString::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find(ACE_TEXT(':'))) == ACE_TString::npos) {
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
  ACE_TString pub_id_str(arg.c_str(), pos);
  ACE_TString pub_addr_str(arg.c_str() + pos + 1);

  this->pub_id_ = ACE_OS::atoi(pub_id_str.c_str());
  this->pub_addr_ = ACE_INET_Addr(pub_addr_str.c_str());

  return 0;
}


int
PubDriver::parse_sub_arg(const ACE_TString& arg)
{
  ACE_TString::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find(ACE_TEXT(':'))) == ACE_TString::npos) {
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
  ACE_TString sub_id_str(arg.c_str(), pos);
  ACE_TString if_addr_str(arg.c_str() + pos + 1);

  this->sub_id_ = ACE_OS::atoi(sub_id_str.c_str());

  // Use the remainder as the "stringified" ACE_INET_Addr.
  // Add in a dummy port # so ACE_INET_Addr can parse the address
  if_addr_str += ACE_TEXT(":0");
  this->if_addr_ = ACE_INET_Addr(if_addr_str.c_str());

  return 0;
}
