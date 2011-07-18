// -*- C++ -*-
//
// $Id$
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"


class Worker
{
public:

  Worker (::DDS::Entity_ptr writer);

  void start ();

  void end ();

  int run_test (const ACE_Time_Value& duration);

private:

  ::DDS::Entity_var writer_;
};

#endif /* WRITER_H */
