#ifndef RTPSRELAY_RELAY_HTTP_META_DISCOVERY_H_
#define RTPSRELAY_RELAY_HTTP_META_DISCOVERY_H_

#include "GuidAddrSet.h"

#include <ace/Acceptor.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Stream.h>
#include <ace/Svc_Handler.h>

namespace RtpsRelay {

const size_t BUFFER_SIZE = 64*1024;

class HttpStatus {
public:
  HttpStatus(int status, const char* message)
    : status_(status)
    , message_(message)
  {}

  int status() const { return status_; }
  const std::string& message() const { return message_; }

private:
  int status_;
  std::string message_;
};

class RelayHttpMetaDiscovery;

// ACE_NULL_SYNCH is appropriate since the reactor is single threaded.
class HttpConnection : public ACE_Svc_Handler<ACE_SOCK_Stream, ACE_NULL_SYNCH> {
public:
  HttpConnection()
    : relay_http_meta_discovery_(0)
    , buffer_(BUFFER_SIZE)
  {}

  int open(void* acceptor); // called by ACE_Acceptor

private:
  int handle_input(ACE_HANDLE h);

  const RelayHttpMetaDiscovery* relay_http_meta_discovery_;
  ACE_Message_Block buffer_;
};

class RelayHttpMetaDiscovery : public ACE_Acceptor<HttpConnection, ACE_SOCK_Acceptor> {
public:
  RelayHttpMetaDiscovery(const Config& config,
                         const std::string& meta_discovery_content_type,
                         const std::string& meta_discovery_content,
                         GuidAddrSet& guid_addr_set)
    : config_(config)
    , meta_discovery_content_type_(meta_discovery_content_type)
    , meta_discovery_content_(meta_discovery_content)
    , guid_addr_set_(guid_addr_set)
  {}

  bool processRequest(ACE_SOCK_Stream& peer,
                      const std::string& request) const;
  bool requestIsComplete(const std::string& request,
                         std::string& target) const;
  bool parseRequestLine(const std::string& requestLine,
                        std::string& target) const;

  void respond(std::stringstream& response) const;
  void respondHealthcheck(std::stringstream& response) const;
  void respondStatus(std::stringstream& response, const HttpStatus& status) const;

private:
  const Config& config_;
  const std::string meta_discovery_content_type_;
  const std::string meta_discovery_content_;
  GuidAddrSet& guid_addr_set_;
};

}

#endif // RTPSRELAY_RELAY_HTTP_META_DISCOVERY_H_
