// -*- C++ -*-
//

#include "StatisticalValue.h"

// Deconflict macros from ACE and STL algorithms.
#ifdef min
#undef min
#endif /* min */

#ifdef max
#undef max
#endif /* max */

#include <stdlib.h>  // For rand() (Uniform, Exponential, Gaussian)
#include <math.h>    // For log() and sqrt() (Exponential, Gaussian)
#include <algorithm> // For max() and min() (Gaussian)

namespace { // Anonymous namespace for file scope.
  /// Random number generator range is from 0 to RAND_MAX.
  const double range = static_cast<double>( RAND_MAX);

} // End of anonymous namespace.

Test::StatisticalImpl::StatisticalImpl()
  : nextValue_(0.0)
{
}

double
Test::StatisticalImpl::uniform(
  double lower,
  double upper
) const
{
  // Scale and shift the system random value.
  return static_cast<double>( lower)
         + ( static_cast<double>(upper - lower)
           * static_cast<double>(rand())/range
           );
}

double
Test::StatisticalImpl::exponential( double mean) const
{
  // Direct functional exponential deviate value.
  if( mean == 0.0) return 0.0;
  return -log( static_cast<double>( rand()) / range) / mean;
}

double
Test::StatisticalImpl::gaussian(
  bool&  dirty,
  double mean,
  double lower,
  double upper,
  double deviation
) const
{
  // Shortcut for fixed values.
  if( deviation == 0.0) {
    return mean;
  }

  // Return previously calculated deviate if we have one.
  if( !dirty) {
    dirty = true;
    return this->nextValue_;
  }

  double x;       /// Uniformly random location on the X-axis.
  double y;       /// Uniformly random location on the Y-axis.
  double s = 0.0; /// Vector magnitude squared.

  // Generate a non-zero random point within the unit circle.
  while( (s == 0.0) || (s >= 1.0)) {
    // Random point in the square [-1,1],[-1,1].
    x = 2.0 * static_cast<double>( rand() / range) - 1.0;
    y = 2.0 * static_cast<double>( rand() / range) - 1.0;

    // Magnitude of the point vector.
    s = x * x + y * y;
  }

  // Box-Muller transformation to standard normal deviate values.
  double factor = sqrt( -2.0 * log( s) / s);

  //
  // Keep one of the two generated deviates.
  //

  // Z-adjusted to the desired mean and deviation.
  double value = ((y * factor) * deviation) + mean;

  //  Bounded by the specified maximum and minimum values.
  dirty = false;
  this->nextValue_ = std::min( upper, std::max( lower, value));

  //
  // And return the other of the two generated deviates.
  //

  // Z-adjusted to the desired mean and deviation.
  value = ((x * factor) * deviation) + mean;

  //  Bounded by the specified maximum and minimum values.
  return std::min( upper, std::max( lower, value));
}

