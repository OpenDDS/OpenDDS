#ifndef _INITERROR_
#define _INITERROR_

#include <string>

class Msg
{
 public:
  InitError (const char* msg);

  const std::string& msg (void) const;

 private:

  const std::string msg_;
};

#endif // _INITERROR_
