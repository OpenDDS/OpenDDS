#ifndef TESTEXCEPTION_H
#define TESTEXCEPTION_H

#include <exception>

class TestException : public virtual std::exception {
  public: virtual const char* what() const throw() { return "TestException"; }
};

class NoInfoRepoException : public virtual TestException {
  public: virtual const char* what() const throw() { return "NoInfoRepo"; }
};

class InconsistentArgumentsException : public virtual TestException {
  public: virtual const char* what() const throw() { return "InconsistentArguments"; }
};

class BadReaderListenerException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadReaderListener"; }
};

class BadWriterListenerException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadWriterListener"; }
};

class BadParticipantException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadParticipant"; }
};

class BadTopicException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadTopic"; }
};

class BadSubscriberException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadSubscriber"; }
};

class BadPublisherException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadPublisher"; }
};

class BadReaderException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadReader"; }
};

class BadWriterException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadWriter"; }
};

class BadTransportException : public virtual TestException {
  public: virtual const char* what() const throw() { return "BadTransport"; }
};

class OutOfRangeException : public virtual TestException {
  public: virtual const char* what() const throw() { return "OutOfRange"; }
};

class NoFileException : public virtual TestException {
  public: virtual const char* what() const throw() { return "NoFile"; }
};

class NoMemoryException : public virtual TestException {
  public: virtual const char* what() const throw() { return "NoMemory"; }
};

class NoJoyException : public virtual TestException {
  public: virtual const char* what() const throw() { return "NoJoy"; }
};

#endif
