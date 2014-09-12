/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TEST_UTILS_SYNC_H
#define TEST_UTILS_SYNC_H

#include "TestUtils_Export.h"

#include "dds/DdsDcpsC.h"

namespace TestUtils {

  /// Helper class for waiting on write events to occur.
  /// This is currently just a wrapper around OpenDDS::Model::WriterSync.
  class TestUtils_Export WriterSync {
  public:
    WriterSync(DDS::DataWriter_var writer, unsigned int num_readers = 1);
    ~WriterSync();
    static int wait_match(const DDS::DataWriter_var& writer, unsigned int num_readers = 1);
    static int wait_ack(const DDS::DataWriter_var& writer);
  private:
    DDS::DataWriter_var writer_;
  };

  /// Helper class for waiting on reader events to occur.
  /// This is currently just a wrapper around OpenDDS::Model::ReaderSync.
  class TestUtils_Export ReaderSync {
  public:
    ReaderSync(DDS::DataReader_var reader, unsigned int num_writers = 1);
    ~ReaderSync();
    static int wait_unmatch(const DDS::DataReader_var& reader, unsigned int num_writers = 1);
  private:
    DDS::DataReader_var reader_;
    unsigned int        num_writers_;
  };
};

#endif
