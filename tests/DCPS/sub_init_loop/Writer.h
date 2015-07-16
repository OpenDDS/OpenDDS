// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>

#include <string>

class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer
    , const char* sub_fin_file_name
    , bool verbose = false
    , int write_delay_ms = 0
    , int num_instances_per_writer = 1);

  bool start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  bool is_finished () const;

  int get_timeout_writes () const;


private:

  ::DDS::DataWriter_var writer_;
  std::string sub_fin_file_name_;
  bool verbose_;
  int write_delay_ms_;
  int num_instances_per_writer_;
  int timeout_writes_;
};

#endif /* WRITER_H */
