#ifndef DDS_DCPS_TEST_INFOREPO_HELPER
#define DDS_DCPS_TEST_INFOREPO_HELPER

namespace OpenDDS
{
  namespace DCPS
  {
    class DataWriterRemote;
    typedef DataWriterRemote* DataWriterRemote_ptr;
    class DataWriterImpl;
    class DataReaderRemote;
    typedef DataReaderRemote* DataReaderRemote_ptr;
    class DataReaderImpl;
  }
}

//This class is granted friendship to DataWriterImpl and DataReaderImpl so we
//can get the remote object references.

class DDS_TEST
{
public:
  static OpenDDS::DCPS::DataWriterRemote_ptr
  getRemoteInterface(const OpenDDS::DCPS::DataWriterImpl &impl);

  static OpenDDS::DCPS::DataReaderRemote_ptr
  getRemoteInterface(const OpenDDS::DCPS::DataReaderImpl &impl);
};

#endif
