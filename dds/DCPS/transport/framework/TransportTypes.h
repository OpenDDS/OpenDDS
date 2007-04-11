#ifndef TRANSPORTAPI_TRANSPORTTYPES_H
#define TRANSPORTAPI_TRANSPORTTYPES_H

#include <string>
#include <vector>
#include <utility>

namespace TransportAPI
{
  /// Request identifier
  typedef unsigned long Id;

  /// An opaque BLOB type
  /// @note The BLOB should contain information related to the current endpoint, and
  /// ideally enough to communicate to a remote endpoint how it should connect to
  /// this one.
  struct BLOB
  {
    size_t size;
    void* data;
  };

  /// Name-value pair support
  typedef std::pair<std::string, std::string> NVP;
  typedef std::vector<NVP> NVPList;

  /// A class to represent reasons for operational failure
  class failure_reason
    : public std::exception
  {
  public:
    explicit failure_reason(const std::string& reason, const Id& id = 0)
      : std::exception()
      , reason_(reason)
      , id_(id)
    {
    }

    failure_reason(const failure_reason& rhs)
      : std::exception()
      , reason_(rhs.reason_)
      , id_(rhs.id_)
    {
    }

    virtual ~failure_reason() throw()
    {
    }

    virtual const char* what() const throw()
    {
      return reason_.c_str();
    }

    const Id& id() const throw()
    {
      return id_;
    }

  private:
    std::string reason_;
    Id id_;
  };

  typedef std::pair<bool, failure_reason> Status;

  // An iovec struct
  struct iovec
  {
    void* iov_base;
    size_t iov_len;
  };
}

#endif /* TRANSPORTAPI_TRANSPORTTYPES_H */
