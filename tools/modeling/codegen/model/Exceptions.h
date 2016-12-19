#ifndef MODEL_EXCEPTIONS_H
#define MODEL_EXCEPTIONS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "model_export.h"
#include <exception>
#include <stdexcept>
#include <dds/Versioned_Namespace.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model {

class Exception : public std::exception {
  public: virtual const char* what() const throw() { return "OpenDDS::Model::Exception"; }
};

class NoServiceException : public Exception {
  public: virtual const char* what() const throw() { return "NoService"; }
};

class NoParticipantException : public Exception {
  public: virtual const char* what() const throw() { return "NoParticipant"; }
};

class NoPublisherException : public Exception {
  public: virtual const char* what() const throw() { return "NoPublisher"; }
};

class NoSubscriberException : public Exception {
  public: virtual const char* what() const throw() { return "NoSubscriber"; }
};

class NoWriterException : public Exception {
  public: virtual const char* what() const throw() { return "NoWriter"; }
};

class NoReaderException : public Exception {
  public: virtual const char* what() const throw() { return "NoReader"; }
};

class NoTopicException : public Exception {
  public: virtual const char* what() const throw() { return "NoTopic"; }
};

class NoTypeException : public Exception {
  public: virtual const char* what() const throw() { return "NoType"; }
};

class NoTransportException : public Exception {
  public: virtual const char* what() const throw() { return "NoTransport"; }
};

class BadConfigureException : public Exception {
  public: virtual const char* what() const throw() { return "BadConfigure"; }
};

class BadAttachException : public Exception {
  public: virtual const char* what() const throw() { return "BadAttach"; }
};

class BadRegisterException : public Exception {
  public: virtual const char* what() const throw() { return "BadRegister"; }
};

class BadCastException : public Exception {
  public: virtual const char* what() const throw() { return "BadCast"; }
};

class OutOfBoundsException : public Exception {
  public: virtual const char* what() const throw() { return "OutOfBounds"; }
};

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* MODEL_EXCEPTIONS_H */

