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

    void run(int& argc, char* argv[]);


  private:

    enum
    {
      TRANSPORT_TYPE_ID,
      TRANSPORT_IMPL_ID
    };

    void parse_args(int& argc, char* argv[]);
    void init();
    void run();

    void parse_arg_n(const char* arg, bool& flag);
    void parse_arg_d(const char* arg, bool& flag);
    void parse_arg_p(const char* arg, bool& flag);
    void parse_arg_s(const char* arg, bool& flag);
    void print_usage(const char* exe_name);
    void required_arg(char opt, bool flag);


    Sub subscriber_;

    ACE_INET_Addr local_address_;
    std::string   sub_addr_str_;
};

#endif
