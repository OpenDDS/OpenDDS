// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>


class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer
          , size_t msg_cnt, size_t sub_cnt);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  bool is_finished () const;

  int get_timeout_writes () const;


private:

  ::DDS::DataWriter_var writer_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;

  size_t msg_cnt_;
  size_t sub_cnt_;
};

#endif /* WRITER_H */
