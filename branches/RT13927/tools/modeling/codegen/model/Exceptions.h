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

namespace OpenDDS { namespace Model {

class OpenDDS_Model_Export Exception : public virtual std::exception {
  public: virtual const char* what() const throw() { return "OpenDDS::Model::Exception"; }
};

class OpenDDS_Model_Export NoParticipantException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoParticipant"; }
};

class OpenDDS_Model_Export NoPublisherException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoPublisher"; }
};

class OpenDDS_Model_Export NoSubscriberException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoSubscriber"; }
};

class OpenDDS_Model_Export NoWriterException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoWriter"; }
};

class OpenDDS_Model_Export NoReaderException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoReader"; }
};

class OpenDDS_Model_Export NoTopicException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoTopic"; }
};

class OpenDDS_Model_Export NoTypeException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoType"; }
};

class OpenDDS_Model_Export NoTransportException : public virtual Exception {
  public: virtual const char* what() const throw() { return "NoTransport"; }
};

class OpenDDS_Model_Export BadConfigureException : public virtual Exception {
  public: virtual const char* what() const throw() { return "BadConfigure"; }
};

class OpenDDS_Model_Export BadAttachException : public virtual Exception {
  public: virtual const char* what() const throw() { return "BadAttach"; }
};

} } // End of namespace OpenDDS::Model

#include /**/ "ace/post.h"

#endif /* MODEL_EXCEPTIONS_H */

