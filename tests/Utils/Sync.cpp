#include "Sync.h"
#include <model/Sync.h>
#include <dds/DCPS/debug.h>
#include <dds/DCPS/WaitSet.h>
#include <stdexcept>

using OpenDDS::DCPS::DCPS_debug_level;

TestUtils::WriterSync::WriterSync(DDS::DataWriter_var writer,
                                       unsigned int num_readers) :
writer_(writer)
{
  if (wait_match(writer_, num_readers)) {
    throw std::runtime_error("wait_match failure");
  }
}

TestUtils::WriterSync::~WriterSync()
{
  if (wait_ack(writer_)) {
    throw std::runtime_error("wait_ack failure");
  }
}

int
TestUtils::WriterSync::wait_match(const DDS::DataWriter_var& writer,
                                       unsigned int num_readers)
{
  return OpenDDS::Model::WriterSync::wait_match(writer, num_readers);
}

int
TestUtils::WriterSync::wait_ack(const DDS::DataWriter_var& writer)
{
  return OpenDDS::Model::WriterSync::wait_ack(writer);
}

TestUtils::ReaderSync::ReaderSync(DDS::DataReader_var reader,
                                       unsigned int num_writers) :
reader_(reader),
num_writers_(num_writers)
{
}

TestUtils::ReaderSync::~ReaderSync()
{
  if (wait_unmatch(reader_, num_writers_)) {
    throw std::runtime_error("wait_unmatch failure");
  }
}

int
TestUtils::ReaderSync::wait_unmatch(const DDS::DataReader_var& reader,
                                         unsigned int num_writers)
{
  return OpenDDS::Model::ReaderSync::wait_unmatch(reader, num_writers);
}
