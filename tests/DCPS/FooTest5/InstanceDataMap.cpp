// -*- C++ -*-
//
#include "InstanceDataMap.h"
#include "common.h"

InstanceDataMap::InstanceDataMap()
: maps_(0),
  num_messages_expected_ (0),
  num_receives_per_sample_ (INT_MAX)
{
}

void InstanceDataMap::init()
{
  num_messages_expected_
    = num_datawriters * num_instances_per_writer * num_samples_per_instance;
  maps_ = new DataMap[num_datawriters];

  num_receives_per_sample_ = num_datareaders;
  if (mixed_trans)
  {
    // We always have a datareader that uses a different transport
    // from other datareaderes for the mixed_trans test.
    num_receives_per_sample_ = num_datareaders - 1;
  }
}


InstanceDataMap::~InstanceDataMap()
{
  delete [] maps_;
}

int
InstanceDataMap::add (Xyz::Foo& sample)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);
  if (sample.x >= num_samples_per_instance)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: reader received unexpected message number %d!\n"),
      sample.x),
      -1);
  }

  if (sample.y >= num_datawriters)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: reader received unexpected message from writer %d!\n"),
      sample.y),
      -1);
  }

  CORBA::ULong length = sample.values.length ();

  if (length != (CORBA::ULong) sequence_length)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: unexpected sequence length %d expected %d!\n"),
      length, sequence_length),
      -1);
  }

  for (CORBA::ULong i = 0; i < length; i ++)
  {
    if (sample.values[i] != sample.x * sample.x - i)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: incorrect sequence %dth value %f expected %f %x!\n"),
        i, sample.values[i], sample.x * sample.x - i, sample.values[i]),
        -1);
    }
  }

  DataMap& map = maps_[(int)sample.y];
  DataMap::ENTRY* entry = 0;
  if (map.find (sample.data_source, entry) == 0)
  {
    if ((entry->int_id_)[(int)sample.x] >= num_receives_per_sample_)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: reader received duplicate message %d ")
        ACE_TEXT("from writer %d!\n"),
        sample.x, sample.y));
      return -1;
    }
    (entry->int_id_)[(int)sample.x] ++;
  }
  else
  {
    int init_val = 0;
    DataArray array (num_samples_per_instance, init_val);
    array[(int)sample.x] ++;
    if( map.bind (sample.data_source, array) != 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: failed to bind instance %d results!\n"),
      sample.data_source),
      -1);
    }
  }

  return 0;
}


int
InstanceDataMap::add (Xyz::FooNoKey& sample)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);
  if (sample.x >= num_samples_per_instance)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: reader received unexpected message number %d!\n"),
      sample.x),
      -1);
  }

  if (sample.y >= num_datawriters)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: reader received unexpected message from writer %d!\n"),
      sample.y),
      -1);
  }

  CORBA::ULong length = sample.values.length ();

  if (length != (CORBA::ULong) sequence_length)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: unexpected sequence length %d expected %d!\n"),
      length, sequence_length),
      -1);
  }

  for (CORBA::ULong i = 0; i < length; i ++)
  {
    if ((int)sample.values[i] != sample.x * sample.x - i)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: incorrect sequence value %d expected %d!\n"),
        sample.values[i], sample.x * sample.x - i),
        -1);
    }
  }

  DataMap& map = maps_[(int)sample.y];
  DataMap::ENTRY* entry = 0;
  if (map.find (sample.data_source, entry) == 0)
  {
    if ((entry->int_id_)[(int)sample.x] >= num_receives_per_sample_)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: reader received duplicate message %d ")
        ACE_TEXT("from writer %d!\n"),
        sample.x, sample.y));
      return -1;
    }
    (entry->int_id_)[(int)sample.x] ++;
  }
  else
  {
    int init_val = 0;
    DataArray array (num_samples_per_instance, init_val);

    array[(int)sample.x] ++;
    if( map.bind (sample.data_source, array) != 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: failed to bind instance %d results!\n"),
      sample.data_source),
      -1);
    }
  }

  return 0;
}


bool
InstanceDataMap::test_passed(int expected)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    false);

  bool expect_receive_all = expected == num_messages_expected_;
  int received_messages = 0;

  bool result = true;

  for (int i = 0; i < num_datawriters; i++)
  {
    DataMap& map = maps_[i];

    if (expect_receive_all
      && map.current_size () != (unsigned) num_instances_per_writer)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: get %d instances expected %d!\n"),
        map.current_size (), num_instances_per_writer));
      return false;
    }

    DataMap::ENTRY* entry = 0;
    for (DataMap::ITERATOR itr(map);
      itr.next(entry);
      itr.advance())
    {
      for (int i = 0; i < num_samples_per_instance; i ++)
      {
        if (expect_receive_all && (entry->int_id_)[i] < num_receives_per_sample_)
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: received %d msg from %d instance ")
            ACE_TEXT("for %d times, expected %d!\n"),
            i, entry->ext_id_, (entry->int_id_)[i], num_receives_per_sample_));
          result = false;
        }
        else
        {
          received_messages ++;
        }
      }
    }
  }

  // If we are not expect receive all messages that the publisher called
  // write() then we make sure we received the number of expected messages
  // which excludes the failed writes.
  if (! expect_receive_all
    && received_messages == expected)
  {
    result = true;
  }

  return result;
}
