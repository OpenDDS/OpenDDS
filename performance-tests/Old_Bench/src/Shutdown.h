// -*- C++ -*-
//
#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include "orbsvcs/Shutdown_Utilities.h"

#include "Process.h"

namespace Test {

class Shutdown : public Shutdown_Functor {
  public:
    Shutdown( Process& process);

    void operator()( int);

  private:
    Process& process_;
};

inline
Shutdown::Shutdown( Process& process)
 : process_( process)
{
}

inline
void
Shutdown::operator()( int)
{
  this->process_.unblock();
}

} // End of namespace Test

#endif // SHUTDOWN_H

