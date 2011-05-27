// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace Messenger;

// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
{
}

// Implementation skeleton destructor
DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

/// This is an allocator that simply uses malloc and free.
/// A real allocator would most likely use a pool of memory.
template<class T, std::size_t N>
class BogusExampleAllocator : public ACE_Allocator
{
public:
    BogusExampleAllocator() {};
    virtual void *malloc (size_t nbytes) { 
        return ACE_OS::malloc(nbytes);
    };
    virtual void free (void *ptr) {
        ACE_OS::free(ptr);
    };

  /// These methods are no-ops.
  virtual void *calloc (size_t nbytes, char initial_value = '\0') 
        {/* no-op */ return (void*)0;};
  virtual void *calloc (size_t n_elem, size_t elem_size, char initial_value = '\0')
        {/* no-op */ return (void*)0;};

  virtual int remove (void)
        {/* no-op */ return -1; };
  virtual int bind (const char *name, void *pointer, int duplicates = 0)
        {/* no-op */ return -1; };
  virtual int trybind (const char *name, void *&pointer)
        {/* no-op */ return -1; };
  virtual int find (const char *name, void *&pointer)
        {/* no-op */ return -1; };
  virtual int find (const char *name)
        {/* no-op */ return -1; };
  virtual int unbind (const char *name)
        {/* no-op */ return -1; };
  virtual int unbind (const char *name, void *&pointer)
        {/* no-op */ return -1; };
  virtual int sync (ssize_t len = -1, int flags = MS_SYNC)
        {/* no-op */ return -1; };
  virtual int sync (void *addr, size_t len, int flags = MS_SYNC)
        {/* no-op */ return -1; };
  virtual int protect (ssize_t len = -1, int prot = PROT_RDWR)
        {/* no-op */ return -1; };
  virtual int protect (void *addr, size_t len, int prot = PROT_RDWR)
        {/* no-op */ return -1; };
#if defined (ACE_HAS_MALLOC_STATS)
  virtual void print_stats (void) const
        {/* no-op */ };
#endif /* ACE_HAS_MALLOC_STATS */
  virtual void dump (void) const
        {/* no-op */ };

private:
    // do not allow copies. - I am not sure this restriction is necessary.
    BogusExampleAllocator(const BogusExampleAllocator&);
    BogusExampleAllocator& operator=(const BogusExampleAllocator&);
};


void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  num_reads_ ++;

  try
    {
      MessageDataReader_var mdr = MessageDataReader::_narrow (reader);
      MessageDataReaderImpl* dr_impl =
        TAO::DCPS::reference_to_servant<MessageDataReaderImpl> (mdr.in());
      if (0 == dr_impl) {
        cerr << "Failed to obtain DataReader Implementation\n" << endl;
        exit(1);
      }

      const CORBA::Long MAX_ELEMS_TO_RETURN = 1;
#if 0
      // type NOT supporting zero-copy read
      ::DDS::SampleInfoSeq the_info(MAX_ELEMS_TO_RETURN);
      MessageSeq the_data (MAX_ELEMS_TO_RETURN);
#else

      // types supporting zero-copy read 
      DDS::SampleInfoSeq the_info(0, MAX_ELEMS_TO_RETURN);

  #if 0
      // This is an example of the user-code providing its
      // own allocator for the sequence of pointers 
      // to the the samples.  
      BogusExampleAllocator<Messenger::Message*,MAX_ELEMS_TO_RETURN> the_allocator;
      MessageZCSeq the_data (0, MAX_ELEMS_TO_RETURN, &the_allocator);
  #else      
      // Note: the default allocator for a ZCSeq is very fast because
      // it will allocate from a pool on the stack and thus avoid
      // a heap allocation but it is limited to a size of
      // DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE.
      // The user might create a pooled allocator at a higher scope
      // (like a class member) that can reuse the allocator's pool 
      // and thus allow for fast and large sequences.
      MessageSeq the_data (0, MAX_ELEMS_TO_RETURN);
  #endif

      //======= begin old std::vector impl ======
      //BogusExampleAllocator<Messenger::Message*,1> allocator;
      // MessageZCSeq<std::allocator<Messenger::Message*> > the_data (1);

      // this does not work because the_data is not of type MessageZCSeq<>
      //MessageZCSeq<BogusExampleAllocator<Messenger::Message*,1> > the_data (1);

      //DataReaderListener.cpp(66) : error C3861: 'the_data': identifier not found, even with argument-dependent lookup
      // because ">>" needs a space like "> >"
      //MessageZCSeq<BogusExampleAllocator<Messenger::Message*,1>> the_data (1);
    
      //======= end old std::vector impl ======
#endif
#if 0
      // Use the zero-copy API to get data buffers directly.
      DDS::ReturnCode_t status = dr_impl->take (the_data
                                                , the_info
                                                , MAX_ELEMS_TO_RETURN
                                                , ::DDS::ANY_SAMPLE_STATE
                                                , ::DDS::ANY_VIEW_STATE
                                                , ::DDS::ANY_INSTANCE_STATE);
#else
      // Use the zero-copy API to get data buffers directly.
      DDS::ReturnCode_t status = dr_impl->read (the_data
                                                , the_info
                                                , MAX_ELEMS_TO_RETURN
                                                , ::DDS::ANY_SAMPLE_STATE
                                                , ::DDS::ANY_VIEW_STATE
                                                , ::DDS::ANY_INSTANCE_STATE);
#endif

      if (status == DDS::RETCODE_OK) {
        void* addr = &(the_data[0]);
        ACE_DEBUG((LM_INFO,"a value at %x / %x -0x10\n", addr, (&(the_data[0].count) )));
        cout << "Message: subject    = " << the_data[0].subject.in() << endl
             << "         subject_id = " << the_data[0].subject_id   << endl
             << "         from       = " << the_data[0].from.in()    << endl
             << "         count      = " << the_data[0].count        << endl
             << "         text       = " << the_data[0].text.in()    << endl;
        cout << "SampleInfo.sample_rank = " << the_info[0].sample_rank << endl;
      } else if (status == DDS::RETCODE_NO_DATA) {
        cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
      } else {
        cerr << "ERROR: read Message: Error: " <<  status << endl;
      }

      // return the loaned data
      dr_impl->return_loan (the_data, the_info);
    }
  catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void DataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_match" << endl;
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::TAO::DCPS::SubscriptionDisconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::TAO::DCPS::SubscriptionReconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::TAO::DCPS::SubscriptionLostStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_connection_deleted (
  DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}
