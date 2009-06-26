#ifndef TESTCONFIG_H
#define TESTCONFIG_H

#include "ace/ace_wchar.h"
#include "ace/SString.h"

#include <vector>
#include <map>
#include <string>

class TestConfig {
  public:
    /**
     * Keys for different transport implementations.
     */

    enum { // Keys used by the TestSystem class.
      PublisherTransport,
      SubscriberTransport
    } SystemTransportKeys;

    enum { // Keys used by the TestMonitor class.
      Repo1Transport,
      Repo2Transport,
      Repo3Transport
    } MonitorTransportKeys;

    /**
     * @brief Construct from the command line.
     *
     * This constructor passes the command line to the DCPS subsystem,
     * which in turn passes the arguments to the ORB initialization, then
     * parses the remainder to determine subsystem behavior:
     *
     *   -v                     - verbose operation
     *   -Samples <number>      - number of samples to emit into the subsystems
     *   -TypeName <string>     - data type name (default "Foo")
     *   -Address <string>      - transport address
     *   -InfoRepo <string>     - InfoRepo repository IOR
     *   -WriterDomain <string> - publisher domain
     *   -ReaderDomain <string> - subscriber domain
     *   -WriterTopic <string>  - publisher topic
     *   -ReaderTopic <string>  - subscriber topic
     *
     * The InfoRepo, WriterDomain, ReaderDomain, WriterTopic and
     * ReaderTopic arguments may be repeated, with the values being
     * accessible internally with an index corresponding to their
     * order of appearance on the command line.
     *
     * This is horribly position dependent.  Mapping domains to repository
     * keys is done by their relative positions on the command line.  The
     * first -InfoRepo value is assigned a repository key value of 0, with
     * subsequent -InfoRepo values taking the next value in order
     * (incremented by 1).  Any domain definitions are then mapped to the
     * currently active repository key.
     */
    TestConfig( int argc, ACE_TCHAR** argv, char** envp);

    /// Virtual destructor.
    virtual ~TestConfig();

    /// Spew indicator.
    bool& verbose();
    bool  verbose() const;

    /// Number of samples to emit.
    int& samples();
    int  samples() const;

    /// Data type name.
    std::string& typeName();
    std::string  typeName() const;

    /// Transport address string.
    ACE_TString& transportAddressName();
    ACE_TString  transportAddressName() const;

    /// InfoRepo IOR.
    std::string& infoRepoIor( int index = 0);
    std::string  infoRepoIor( int index = 0) const;
    int          infoRepoIorSize() const;

    /// Topic subscribed to.
    std::string& readerTopicName( int index = 0);
    std::string  readerTopicName( int index = 0) const;
    int          readerTopicNameSize() const;

    /// Topic published.
    std::string& writerTopicName( int index = 0);
    std::string  writerTopicName( int index = 0) const;
    int          writerTopicNameSize() const;

    /// Domain subscribed in.
    long& subscriberDomain( int index = 0);
    long  subscriberDomain( int index = 0) const;
    int   subscriberDomainSize() const;

    /// Domain published in.
    long& publisherDomain( int index = 0);
    long  publisherDomain( int index = 0) const;
    int   publisherDomainSize() const;

    /// Mapping of domains to repositories.
    long& domainToRepo( int domain = 0);
    long  domainToRepo( int domain = 0) const;
    int   domainToRepoSize() const;

  private:
    /// Storage type for configured names.
    typedef std::vector<std::string> StringVector;

    /// Storage type for configured domains.
    typedef std::vector<long> DomainVector;

    /// Storage type for configured domains.
    typedef std::map< long, long> DomainRepoMap;

    /// Be chatty while executing.
    bool verbose_;

    /// Number of samples to emit into the subsystems.
    int samples_;

    /// Data type name.
    std::string typeName_;

    /// Transport address string.
    ACE_TString transportAddressName_;

    /// InfoRepo IOR.
    StringVector infoRepoIor_;

    /// Topic subscribed to.
    StringVector readerTopicName_;

    /// Topic published.
    StringVector writerTopicName_;

    /// Domain subscribed in.
    DomainVector subscriberDomain_;

    /// Domain published in.
    DomainVector publisherDomain_;

    /// Map domains to repository keys.
    DomainRepoMap domainRepoMap_;
};

inline
int
TestConfig::infoRepoIorSize() const
{
  return this->infoRepoIor_.size();
}

inline
int
TestConfig::readerTopicNameSize() const
{
  return this->readerTopicName_.size();
}

inline
int
TestConfig::writerTopicNameSize() const
{
  return this->writerTopicName_.size();
}

inline
int
TestConfig::subscriberDomainSize() const
{
  return this->subscriberDomain_.size();
}

inline
int
TestConfig::publisherDomainSize() const
{
  return this->publisherDomain_.size();
}

inline
int
TestConfig::domainToRepoSize() const
{
  return this->domainRepoMap_.size();
}

#endif // TESTCONFIG_H

