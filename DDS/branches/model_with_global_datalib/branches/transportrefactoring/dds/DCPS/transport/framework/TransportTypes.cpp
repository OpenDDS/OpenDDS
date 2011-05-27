#include "DCPS/DdsDcps_pch.h"
#include "TransportTypes.h"

namespace TransportAPI
{
  BLOB::~BLOB()
  {
  }

  const std::string&
  BLOB::getIdentifier() const
  {
    return identifier_;
  }

  void
  BLOB::setIdentifier(const std::string& identifier)
  {
    identifier_ = identifier;
  }
}
