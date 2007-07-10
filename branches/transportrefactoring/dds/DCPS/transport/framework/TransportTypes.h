#ifndef TRANSPORTAPI_TRANSPORTTYPES_H
#define TRANSPORTAPI_TRANSPORTTYPES_H

#include "dds/DCPS/dcps_export.h"
#include <ace/OS_NS_sys_uio.h>
#include <string>
#include <vector>
#include <utility>

namespace TransportAPI
{
  /// Request identifier
  typedef unsigned long Id;

  /// Name-value pair support
  typedef std::pair<std::string, std::string> NVP;
  typedef std::vector<NVP> NVPList;

  /// An opaque BLOB type
  /// @note The BLOB should contain information related to the current endpoint, and
  /// ideally enough to communicate to a remote endpoint how it should connect to
  /// this one.
  class OpenDDS_Dcps_Export BLOB
  {
  public:
    virtual ~BLOB();

    const std::string& getIdentifier() const;

  protected:
    void setIdentifier(const std::string& identifier);

  private:
    std::string identifier_;
  };

  /// A class to represent reasons for operational failure
  class failure_reason
    : public std::exception
  {
  public:
    explicit failure_reason(const std::string& reason = std::string(), const Id& id = 0)
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

  enum Resolution
  {
    SUCCESS,
    FAILURE,
    DEFERRED
  };

  typedef std::pair<Resolution, failure_reason> Status;

  inline Status
  make_success(const failure_reason& reason = failure_reason())
  {
    return std::make_pair(SUCCESS, reason);
  }

  inline Status
  make_failure(const failure_reason& reason = failure_reason())
  {
    return std::make_pair(FAILURE, reason);
  }

  inline Status
  make_deferred(const failure_reason& reason = failure_reason())
  {
    return std::make_pair(DEFERRED, reason);
  }
}

#endif /* TRANSPORTAPI_TRANSPORTTYPES_H */
