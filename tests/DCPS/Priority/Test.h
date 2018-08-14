// -*- C++ -*-
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <exception>

namespace Test {
  /// Delay (seconds) to wait for a subscription to be made at test start.
  enum { SUBSCRIPTION_WAIT_TIME = 30};

  /// Delay (seconds) to wait for a publication to publish.
  enum { PUBLICATION_WAIT_TIME = 30};

  /// Heartbeat interval for publications.
  enum { PUBLICATION_LIVELINESS_INTERVAL = 5 };

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

