namespace TAO
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
  static TAO::DCPS::DataWriterRemote_ptr
  getRemoteInterface(const TAO::DCPS::DataWriterImpl &impl);

  static TAO::DCPS::DataReaderRemote_ptr
  getRemoteInterface(const TAO::DCPS::DataReaderImpl &impl);
};
