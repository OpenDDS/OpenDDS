#ifndef SYNC_H
#define SYNC_H

#include "model_export.h"
#include "dds/DdsDcpsC.h"

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
  };
};

#endif
