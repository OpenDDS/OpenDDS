#include "InitError.h"

InitError::InitError (const char* msg)
  : msg_ (msg)
{
}

const std::string&
InitError::msg (void) const
{
  return msg_;
}
