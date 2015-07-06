// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"


class Writer
{
public:

  Writer (::DDS::DataWriter_ptr writer);

  void start ();

  void end ();

  int run_test (const ACE_Time_Value& duration);

private:

  ::DDS::DataWriter_var writer_;
};

#endif /* WRITER_H */
