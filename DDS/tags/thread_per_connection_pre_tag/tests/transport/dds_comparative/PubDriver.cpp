// -*- C++ -*-
//
// $Id$

#include "PubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include <ace/Arg_Shifter.h>
#include <string>


PubDriver::PubDriver()
{
}


PubDriver::~PubDriver()
{
}


void
PubDriver::run(int& argc, char* argv[])
{
  this->parse_args(argc, argv);
  this->init();
  this->run();
}


void
PubDriver::parse_args(int& argc, char* argv[])
{
  // Command-line arguments:
  //
  // -n <num samples to send>
  // -d <data size>
  // -p <pub_id:pub_host:pub_port>
  // -s <sub_id:sub_host:sub_port>
  //
  // The -s option should be specified for each remote subscriber.
  //

  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool flag_n = false;
  bool flag_d = false;
  bool flag_p = false;
  bool flag_s = false;

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ((current_arg = arg_shifter.get_the_parameter("-n")))
        {
          this->parse_arg_n(current_arg, flag_n);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-d")))
        {
          this->parse_arg_d(current_arg, flag_d);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-p")))
        {
          this->parse_arg_p(current_arg, flag_p);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-s")))
        {
          this->parse_arg_s(current_arg, flag_s);
          arg_shifter.consume_arg();
        }
      else if (arg_shifter.cur_arg_strncasecmp("-?") == 0)
        {
          this->print_usage(argv[0]);
          arg_shifter.consume_arg();
          throw TestException();
        }
      else
        {
          arg_shifter.ignore_arg();
        }
    }

  this->required_arg('n', flag_n);
  this->required_arg('d', flag_d);
  this->required_arg('p', flag_p);
  this->required_arg('s', flag_s);
}


void
PubDriver::init()
{
  // Ask TheTransportFactory to create a TransportImpl using the
  // "SimpleTcp" TransportImplFactory object that was registered as default
  // SimpleTcp factory. We also assign an impl id (aka instance id) to
  // the TransportImpl object that gets created - the impl id we assign
  // to this TransportImpl object is TRANSPORT_IMPL_ID.  After the
  // create_transport_impl() has completed, the 
  // obtain() would give a "copy" of the cached reference to the 
  // TransportImpl object.
  TAO::DCPS::TransportImpl_rch transport_impl 
    = TheTransportFactory->create_transport_impl (TRANSPORT_IMPL_ID, 
                                                  "SimpleTcp",
                                                  TAO::DCPS::DONT_AUTO_CONFIG);
  // Now we can configure the TransportImpl object.
  TAO::DCPS::TransportConfiguration_rch config 
    = TheTransportFactory->create_configuration (TRANSPORT_IMPL_ID, "SimpleTcp");
      
  TAO::DCPS::SimpleTcpConfiguration* tcp_config 
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (config.in ());

  // We use all of the default configuration settings, except for those
  // that need to be set (ie, the default values are not valid).

  // The only setting that falls into this category is the local address,
  // which we have saved in a data member.
  tcp_config->local_address_ = this->local_address_;

  // Supply the config object to the TransportImpl object.
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
  // This will cause the publisher to attach to the transport and add
  // all of the appropriate subscriptions.
  this->publisher_.init(TRANSPORT_IMPL_ID);

  // This will cause the publisher to send all of the messages to
  // the transport.
  this->publisher_.run();

  // This will block until the publisher has received confirmation
  // from the transport that all messages have been sent.
  this->publisher_.wait();

  // We can release TheTransportFactory now, causing all registered
  // TransportImplFactory objects (references) to be dropped, and
  // all TransportImpl objects (references) to be shutdown() and then
  // dropped.
  TheTransportFactory->release();
}


void
PubDriver::parse_arg_n(const char* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -n allowed on command-line.\n"));
      throw TestException();
    }

  int value = ACE_OS::atoi(arg);

  if (value <= 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -n option must be > 0.\n"));
      throw TestException();
    }

  this->publisher_.set_num_to_send(value);

  flag = true;
}


void
PubDriver::parse_arg_d(const char* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -d allowed on command-line.\n"));
      throw TestException();
    }

  int value = ACE_OS::atoi(arg);

  if (value <= 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -d option must be > 0.\n"));
      throw TestException();
    }

  if (value > 32)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -d option must be < 32.\n"));
      throw TestException();
    }

  char data_size = value;
  this->publisher_.set_data_size(data_size);

  flag = true;
}


void
PubDriver::parse_arg_p(const char* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -p allowed on command-line.\n"));
      throw TestException();
    }

  std::string arg_str = arg;
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg_str.find_first_of(':')) == std::string::npos)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "Missing ':' chars.\n",
                 arg));
      throw TestException();
    }

  if (pos == 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "':' char cannot be first char.\n",
                 arg));
      throw TestException();
    }

  if (pos == arg_str.length() - 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "':' char cannot be last char.\n",
                 arg));
      throw TestException();
    }

  // Parse the pub_id from left of ':' char, and remainder to right of ':'.
  std::string pub_id_str(arg_str,0,pos);
  std::string pub_addr_str(arg_str,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  TAO::DCPS::RepoId pub_id = ACE_OS::atoi(pub_id_str.c_str());

  this->local_address_ = ACE_INET_Addr(pub_addr_str.c_str());

  this->publisher_.set_local_publisher(pub_id);
  
  flag = true;
}


void
PubDriver::parse_arg_s(const char* arg, bool& flag)
{
  std::string arg_str = arg;
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg_str.find_first_of(':')) == std::string::npos)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "Missing ':' chars.\n",
                 arg));
      throw TestException();
    }

  if (pos == 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "':' char cannot be first char.\n",
                 arg));
      throw TestException();
    }

  if (pos == arg_str.length() - 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "':' char cannot be last char.\n",
                 arg));
      throw TestException();
    }

  // Parse the sub_id from left of ':' char, and remainder to right of ':'.
  std::string sub_id_str(arg_str,0,pos);
  std::string sub_addr_str(arg_str,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  TAO::DCPS::RepoId sub_id = ACE_OS::atoi(sub_id_str.c_str());

  ACE_INET_Addr sub_addr(sub_addr_str.c_str());

  this->publisher_.add_remote_subscriber(sub_id,sub_addr);

  flag = true;
}


void
PubDriver::print_usage(const char* exe_name)
{
  ACE_DEBUG((LM_DEBUG,
             "Usage for executable: %s\n\n"
             "    -n num_msgs_to_send\n"
             "    -d data_size\n"
             "    -p pub_id:pub_host:pub_port\n"
             "    -s sub_id:sub_host:sub_port\n",
             exe_name));
}


void
PubDriver::required_arg(char opt, bool flag)
{
  if (!flag)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Missing required command-line option: -%c.\n",
                 opt));
      throw TestException();
    }
}
