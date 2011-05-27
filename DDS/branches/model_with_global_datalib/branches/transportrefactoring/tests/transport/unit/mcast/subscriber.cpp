#include "dds/DCPS/transport/MCast/MCastTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>
#include <ace/Condition_Thread_Mutex.h>

static const char* sig    = "ready.txt";
static const char* output = "data.txt";
static const char* host   = ACE_DEFAULT_MULTICAST_ADDR;
static const char* port   = "12345";
static const char* remoteHost   = ACE_DEFAULT_MULTICAST_ADDR;
static const char* remotePort   = "12345";

static int
parse_args(int argc, char** argv)
{
  ACE_Get_Opt get_opts(argc, argv, "d:h:p:r:s:");
  int c;

  while ((c = get_opts()) != -1) {
    switch(c) {
      case 'd':
        output = get_opts.opt_arg();
        break;
      case 'h':
        host = get_opts.opt_arg();
        break;
      case 'p':
        port = get_opts.opt_arg();
        break;
      case 'r':
        remoteHost = get_opts.opt_arg();
        break;
      case 's':
        remotePort = get_opts.opt_arg();
        break;
      case '?':
      default:
        ACE_ERROR_RETURN((LM_ERROR,
                          "usage:  %s "
                          "-d <output> "
                          "-h <host> "
                          "-p <port> "
                          "-r <host> "
                          "-s <port> "
                          "\n",
                          argv [0]),
                         -1);
    }
  }

  return 0;
}

class DataSink: public TransportAPI::LinkCallback
{
public:
  DataSink();
  virtual ~DataSink();

  int checkStatus() const;
  void writeData(const char* file);

  virtual void connected(const TransportAPI::Id& requestId);
  virtual void disconnected(const TransportAPI::failure_reason& reason);
  virtual void sendSucceeded(const TransportAPI::Id& requestId);
  virtual void sendFailed(const TransportAPI::failure_reason& reason);
  virtual void backPressureChanged(bool applyBackpressure,
                                   const TransportAPI::failure_reason& reason);
  virtual void received(const iovec buffers[], size_t iovecSize);

  void wait();

private:
  bool received_;
  bool written_;

  typedef std::vector<std::pair<size_t, char*> > Buffers;
  Buffers buffers_;
};

DataSink::DataSink()
 : received_(false),
   written_(false)
{
}

DataSink::~DataSink()
{
  size_t maxBuf = buffers_.size();
  for(size_t i = 0; i < maxBuf; i++) {
    delete [] buffers_[i].second;
  }
}

int
DataSink::checkStatus() const
{
  if (received_ && written_) {
    return 0;
  }
  return 1;
}

void
DataSink::writeData(const char* file)
{
  FILE* fp = ACE_OS::fopen(file, "w");
  if (fp != 0) {
    written_ = true;
    size_t maxBuf = buffers_.size();
    for(size_t i = 0; i < maxBuf; i++) {
      ACE_OS::fwrite(buffers_[i].second, buffers_[i].first, 1, fp);
    }
    ACE_OS::fclose(fp);
  }
}

void
DataSink::wait()
{
  ACE_OS::sleep(3);
}

void
DataSink::connected(const TransportAPI::Id&)
{
}

void
DataSink::disconnected(const TransportAPI::failure_reason&)
{
}

void
DataSink::sendSucceeded(const TransportAPI::Id&)
{
}

void
DataSink::sendFailed(const TransportAPI::failure_reason&)
{
}

void
DataSink::backPressureChanged(bool,
                              const TransportAPI::failure_reason&)
{
}

void
DataSink::received(const iovec buffer[], size_t iovecSize)
{
  received_ = true;
  for(size_t i = 0; i < iovecSize; i++) {
    char* data = new char[buffer[i].iov_len];
    ACE_OS::memcpy(data, buffer[i].iov_base, buffer[i].iov_len);
    buffers_.push_back(std::pair<size_t, char*>(buffer[i].iov_len, data));
  }
}

int main(int argc, char** argv)
{
  if (parse_args(argc, argv) != 0) {
    return 1;
  }

  TransportAPI::NVPList params;
  params.push_back(TransportAPI::NVP("hostname", host));
  params.push_back(TransportAPI::NVP("port", port));
  params.push_back(TransportAPI::NVP("remoteHostname", remoteHost));
  params.push_back(TransportAPI::NVP("remotePort", remotePort));

  MCastTransport transport;
  TransportAPI::Status status = transport.configure(params);
  if (status.first != TransportAPI::SUCCESS) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Configuration of the transport failed\n"
                     "       %s\n", status.second.what()), 1);
  }

  const TransportAPI::BLOB* endpoint = 0;
  transport.getBLOB(endpoint);
  if (endpoint == 0) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Unable to get the endpoint\n"), 1);
  }

  DataSink sink;
  TransportAPI::Id id = 0;
  std::pair<TransportAPI::Status, TransportAPI::Transport::Link*> pr =
    transport.establishLink(endpoint, id++, &sink, true);
  if (pr.first.first != TransportAPI::SUCCESS) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Unable to establish the link\n"
                     "       %s\n", pr.first.second.what()), 1);
  }

  TransportAPI::Transport::Link* link = pr.second;
  if (link == 0) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Unable to get a link\n"), 1);
  }

  // Create this file so the test script knows to
  // start the publisher
  FILE* fp = ACE_OS::fopen(sig, "w");
  fprintf(fp, "Ready\n");
  ACE_OS::fclose(fp);

  sink.wait();

  status = link->shutdown(id++);
  if (status.first != TransportAPI::SUCCESS) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Unable to shutdown the link\n"
                     "       %s\n", status.second.what()), 1);
  }

  transport.destroyLink(link);

  sink.writeData(output);
  return sink.checkStatus();
}
