#ifndef SYNC_H
#define SYNC_H

#include "model_export.h"
#include "dds/DdsDcpsC.h"
#include <ace/Condition_T.h>

namespace OpenDDS {
  namespace Model {
    class OpenDDS_Model_Export WriterSync {
    public:
      WriterSync(DDS::DataWriter_var& writer);
      ~WriterSync();
      static int wait_match(DDS::DataWriter_var& writer);
      static int wait_ack(DDS::DataWriter_var& writer);
    private:
      DDS::DataWriter_var& writer_;
    };

    class OpenDDS_Model_Export ReaderSync {
    public:
      ReaderSync(DDS::DataReader_var& reader);
      ~ReaderSync();
      static int wait_unmatch(DDS::DataReader_var& reader);
    private:
      DDS::DataReader_var& reader_;
    };

    class OpenDDS_Model_Export ReaderCondSync {
    public:
      ReaderCondSync(DDS::DataReader_var& reader,
                     ACE_Condition<ACE_SYNCH_MUTEX>& condition);
      ~ReaderCondSync();
      void signal();
    private:
      DDS::DataReader_var& reader_;
      bool complete_;
      ACE_Condition<ACE_SYNCH_MUTEX>& condition_;
    };
  };
};

#endif
