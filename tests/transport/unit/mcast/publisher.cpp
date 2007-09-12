#include "dds/DCPS/transport/MCast/MCastTransport.h"
#include "dds/DCPS/transport/framework/LinkCallback.h"
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Get_Opt.h>

static const char* host = ACE_DEFAULT_MULTICAST_ADDR;
static const char* port = "12345";
static const char* remoteHost = ACE_DEFAULT_MULTICAST_ADDR;
static const char* remotePort = "12345";

static int
parse_args(int argc, char** argv)
{
  ACE_Get_Opt get_opts(argc, argv, "h:p:r:s:");
  int c;

  while ((c = get_opts()) != -1) {
    switch(c) {
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

  int checkStatus() const;

  virtual void connected(const TransportAPI::Id& requestId);
  virtual void disconnected(const TransportAPI::failure_reason& reason);
  virtual void sendSucceeded(const TransportAPI::Id& requestId);
  virtual void sendFailed(const TransportAPI::failure_reason& reason);
  virtual void backPressureChanged(bool applyBackpressure,
                                   const TransportAPI::failure_reason& reason);
  virtual void received(const iovec buffers[], size_t iovecSize);

private:
  bool received_;
};

DataSink::DataSink()
 : received_(false)
{
}

int
DataSink::checkStatus() const
{
  if (!received_) {
    return 0;
  }
  return 1;
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
DataSink::received(const iovec[], size_t)
{
  received_ = true;
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
  params.push_back(TransportAPI::NVP("active", "1"));

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
    transport.establishLink(endpoint, id++, &sink, false);
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

  char buffer[1024];
  FILE* fp = ACE_OS::fopen(__FILE__, "r");
  if (fp == 0) {
    ACE_ERROR_RETURN((LM_ERROR, "Unable to open %s\n", __FILE__), 1);
  }

  while(!feof(fp)) {
    size_t ramount = (ACE_OS::rand() % 1023) + 1;
    size_t amount = ACE_OS::fread(buffer, 1, ramount, fp);
    if (amount > 0) {
      iovec buf[1];
      buf[0].iov_base = buffer;
      buf[0].iov_len = amount;
      link->send(buf, 1, id++);
    }
  }
  ACE_OS::fclose(fp);

  status = link->shutdown(id++);
  if (status.first != TransportAPI::SUCCESS) {
   ACE_ERROR_RETURN((LM_ERROR,
                     "ERROR: Unable to shutdown the link\n"
                     "       %s\n", status.second.what()), 1);
  }
  transport.destroyLink(link);
  return sink.checkStatus();
}
