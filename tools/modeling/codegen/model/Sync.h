#ifndef SYNC_H
#define SYNC_H

#include "model_export.h"
#include "dds/DdsDcpsC.h"

#include <ace/Condition_T.h>
#include <ace/Condition_Thread_Mutex.h>

#if defined (ACE_HAS_CPP11)
#define OPENDDS_NOEXCEPT_FALSE noexcept(false)
#else
#define OPENDDS_NOEXCEPT_FALSE
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace Model {
    class OpenDDS_Model_Export WriterSync {
    public:
      WriterSync(DDS::DataWriter_var writer, unsigned int num_readers = 1);
      ~WriterSync() OPENDDS_NOEXCEPT_FALSE;
      static int wait_match(const DDS::DataWriter_var& writer, unsigned int num_readers = 1);
      static int wait_ack(const DDS::DataWriter_var& writer);

      //helper method to force writer to wait for publication_matched to go to 0
      static int wait_unmatch(const DDS::DataWriter_var& writer, unsigned int num_readers = 1);
    private:
      DDS::DataWriter_var writer_;
    };

    class OpenDDS_Model_Export ReaderSync {
    public:
      ReaderSync(DDS::DataReader_var reader, unsigned int num_writers = 1);
      ~ReaderSync() OPENDDS_NOEXCEPT_FALSE;
      static int wait_unmatch(const DDS::DataReader_var& reader, unsigned int num_writers = 1);
    private:
      DDS::DataReader_var reader_;
      unsigned int        num_writers_;
    };

    class OpenDDS_Model_Export ReaderCondSync {
    public:
      ReaderCondSync(DDS::DataReader_var reader,
                     ACE_Condition<ACE_SYNCH_MUTEX>& condition);
      ~ReaderCondSync();
      void signal();
    private:
      DDS::DataReader_var reader_;
      bool complete_;
      ACE_Condition<ACE_SYNCH_MUTEX>& condition_;
    };
  };
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
