// -*- C++ -*-
//
// $Id$
#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "Sub.h"
#include "dds/DCPS/Definitions.h"
#include "ace/INET_Addr.h"


class SubDriver
{
  public:

    SubDriver();
    virtual ~SubDriver();

    void run(int& argc, ACE_TCHAR* argv[]);


  private:

    enum
    {
      TRANSPORT_TYPE_ID,
      TRANSPORT_IMPL_ID
    };

    void parse_args(int& argc, ACE_TCHAR* argv[]);
    void init();
    void run();

    void parse_arg_n(const ACE_TCHAR* arg, bool& flag);
    void parse_arg_d(const ACE_TCHAR* arg, bool& flag);
    void parse_arg_p(const ACE_TCHAR* arg, bool& flag);
    void parse_arg_s(const ACE_TCHAR* arg, bool& flag);
    void print_usage(const ACE_TCHAR* exe_name);
    void required_arg(ACE_TCHAR opt, bool flag);


    Sub subscriber_;

    ACE_INET_Addr local_address_;
    ACE_TString   sub_addr_str_;
};

#endif
