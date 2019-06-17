// -*- C++ -*-
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */
#include <exception>

namespace Test {

  class Exception : public virtual std::exception {
    public: virtual const char* what() const throw() { return "TestException"; }
  };

  class BadParticipantException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadParticipant"; }
  };

  class BadTransportException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadTransport"; }
  };

  class BadTypeSupportException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadTypeSupport"; }
  };

  class BadTopicException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadTopic"; }
  };

  class BadPublisherException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadPublisher"; }
  };

  class BadSubscriberException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadSubscriber"; }
  };

  class BadServantException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadServant"; }
  };

  class BadAttachException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadAttach"; }
  };

  class BadQosException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadQos"; }
  };

  class BadWriterException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadWriter"; }
  };

  class BadReaderException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadReader"; }
  };

  class BadSyncException : public virtual Exception {
    public: virtual const char* what() const throw() { return "BadSync"; }
  };

} // End of namespace Test

