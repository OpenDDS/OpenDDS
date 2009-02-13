// -*- C++ -*-
//
// $Id$

#include "Exponential.h"

#include <stdlib.h>  // For rand()
#include <math.h>    // For log()

namespace { // Anonymous namespace for file scope.
  /// Random number generator range is from 0 to RAND_MAX.
  const double range = static_cast<double>( RAND_MAX);

} // End of anonymous namespace.

namespace Test {

double
Exponential::value() const
{
  // No value if no rate.
  if( this->rate_ == 0) return 0.0;

  // Return an exponentially distributed value.
  return -log( static_cast<double>( rand()) / range) / this->rate_;
}

} // End of namespace Test

