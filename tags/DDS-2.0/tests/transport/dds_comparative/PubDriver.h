// -*- C++ -*-
//
// $Id$
#ifndef PUBDRIVER_H
#define PUBDRIVER_H

#include "Pub.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"


class PubDriver
{
  public:

    /// Default Ctor
    PubDriver();

    /// Dtor
    ~PubDriver();

    /// The only public method (other than ctor/dtor) to "run" the
    /// application.  Called by the process' main() function.
    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    /// Used with TheTransportFactory to register a TransportImplFactory
    /// object, assigning the TRANSPORT_TYPE_ID to the type.
    /// Used with TheTransportFactory to create/obtain the single instance
    /// of a TransportImpl that is needed for this app.
    enum
    {
      TRANSPORT_TYPE_ID,
      TRANSPORT_IMPL_ID
    };

    /// First method called by our public run(argc,argv) method.
    void parse_args(int& argc, ACE_TCHAR* argv[]);

    /// Second method called by our public run(argc,argv) method.
    void init();

    /// Third method called by our public run(argc,argv) method.
    void run();


    /// Helper method invoked by parse_args() when a '-n arg' is encountered
    /// on the command-line.
    void parse_arg_n(const ACE_TCHAR* arg, bool& flag);

    /// Helper method invoked by parse_args() when a '-d arg' is encountered
    /// on the command-line.
    void parse_arg_d(const ACE_TCHAR* arg, bool& flag);

    /// Helper method invoked by parse_args() when a '-p arg' is encountered
    /// on the command-line.
    void parse_arg_p(const ACE_TCHAR* arg, bool& flag);

    /// Helper method invoked by parse_args() when a '-s arg' is encountered
    /// on the command-line.
    void parse_arg_s(const ACE_TCHAR* arg, bool& flag);

    /// Helper method to dump the usage statement.
    void print_usage(const ACE_TCHAR* exe_name);

    /// Helper method to make sure an arg was supplied via cmd-line.
    void required_arg(ACE_TCHAR opt, bool flag);


    /// The publisher object.  The Pub class derives from TransportInterface.
    Pub publisher_;

    /// The Local Address to be used to configure the single TransportImpl
    /// instance that will be used by our (single instance) Pub object.
    ACE_INET_Addr local_address_;
    ACE_TString   pub_addr_str_;
};

#endif
