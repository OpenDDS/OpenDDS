#include "RelayHttpMetaDiscovery.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/ThreadStatusManager.h>

#include <ace/INET_Addr.h>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

namespace RtpsRelay {

const HttpStatus HTTP_OK(200, "OK");
const HttpStatus HTTP_NOT_FOUND(404, "Not Found");
const HttpStatus HTTP_SERVICE_UNAVAILABLE(503, "Service Unavailable");

const int HANDLER_ERROR = -1, HANDLER_REMOVE = -1, HANDLER_OK = 0;

int HttpConnection::open(void* x)
{
  relay_http_meta_discovery_ = static_cast<RelayHttpMetaDiscovery*>(x);

  if (HANDLER_ERROR == reactor()->register_handler(this, READ_MASK)) {
    return HANDLER_ERROR;
  }
  return HANDLER_OK;
}

int HttpConnection::handle_input(ACE_HANDLE)
{
  OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  const auto bytes = peer().recv(buffer_.wr_ptr(), buffer_.space(),
                                 &ACE_Time_Value::zero);

  if (bytes == 0) {
    peer().close_reader();
    return HANDLER_REMOVE;
  }

  if (bytes < 0 && errno == ETIME) {
    return HANDLER_OK;
  }

  if (bytes <= 0) {
    return HANDLER_ERROR;
  }

  buffer_.wr_ptr(bytes);
  const std::string request(buffer_.rd_ptr(), buffer_.length());
  if (relay_http_meta_discovery_->processRequest(peer(), request)) {
    peer().close_writer();
    buffer_.reset();
  }

  return HANDLER_OK;
}

bool RelayHttpMetaDiscovery::processRequest(ACE_SOCK_Stream& peer,
                                            const std::string& request) const
{
  std::string target;
  if (requestIsComplete(request, target)) {
    std::stringstream response;
    if (target == "/config") {
      respond(response);
    } else if (target == "/healthcheck") {
      respondHealthcheck(response);
    } else {
      respondStatus(response, HTTP_NOT_FOUND);
    }
    const std::string& r = response.str();
    peer.send(r.data(), r.size());
    if (config_.log_http()) {
      ACE_INET_Addr remote;
      if (peer.get_remote_addr(remote) == 0) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: Request from %C\n%C\n%C\n"), OpenDDS::DCPS::LogAddr(remote).c_str(), request.c_str(), r.c_str()));
      }
    }
    return true;
  }
  return false;
}

bool RelayHttpMetaDiscovery::parseRequestLine(const std::string& requestLine,
                                              std::string& target) const
{
  // Only GET is supported
  static const char METHOD_GET[] = "GET ";
  static const size_t GET_LEN = sizeof(METHOD_GET) - 1 /* no nul terminator */;
  if (requestLine.find(METHOD_GET) != 0) return false;

  const auto targetEnd = requestLine.find(' ', GET_LEN);
  if (targetEnd == std::string::npos ||
      requestLine.find("HTTP/1.", targetEnd + 1) != targetEnd + 1) {
    return false;
  }

  target = std::string(requestLine, GET_LEN, targetEnd - GET_LEN);
  return true;
}

bool RelayHttpMetaDiscovery::requestIsComplete(const std::string& request,
                                               std::string& target) const
{
  const auto reqLineEnd = request.find("\r\n");
  if (reqLineEnd == std::string::npos ||
      !parseRequestLine(std::string(request, 0, reqLineEnd), target)) {
    return false;
  }
  return request.find("\r\n\r\n") != std::string::npos;
}

void RelayHttpMetaDiscovery::respond(std::stringstream& response) const
{
  response << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << meta_discovery_content_type_ << "\r\n"
           << "Content-Length: " << meta_discovery_content_.size() << "\r\n"
           << "\r\n"
           << meta_discovery_content_;
}

void RelayHttpMetaDiscovery::respondHealthcheck(std::stringstream& response) const
{
  GuidAddrSet::Proxy proxy(guid_addr_set_);
  respondStatus(response, proxy.admitting() ? HTTP_OK : HTTP_SERVICE_UNAVAILABLE);
}

void RelayHttpMetaDiscovery::respondStatus(std::stringstream& response,
                                           const HttpStatus& status) const
{
  response << "HTTP/1.1 " << status.status() << " " << status.message() << "\r\n"
           << "Content-Type: text/plain\r\n"
           << "Content-Length: " << status.message().size() << "\r\n"
           << "\r\n"
           << status.message();
}

}
