// -*- C++ -*-
//

#include "dds/DCPS/debug.h"
#include "PublicationProfile.h"

// Deconflict macros from ACE and STL algorithms.
#ifdef min
#undef min
#endif /* min */

#ifdef max
#undef max
#endif /* max */

#include <stdlib.h>  // For rand()
#include <math.h>    // For log() and sqrt()
#include <algorithm> // for max() and min()

namespace { // Anonymous namespace for file scope.
  /// Random number generator range is from 0 to RAND_MAX.
  const double range = static_cast<double>( RAND_MAX);

} // End of anonymous namespace.

namespace Test {

PublicationProfile::PublicationProfile(
  const std::string& name,
  int priority,
  int rate,
  int size,
  int deviation,
  int max,
  int min

) : name_( name),
    priority_( priority),
    rate_( static_cast<double>( rate)),
    mean_( static_cast<double>( size)),
    deviation_( static_cast<double>( deviation)),
    nextSize_(0),
    sizeAvailable_( false),
    max_( max),
    min_( min)
{
  // Seed here if you want to.
}

PublicationProfile::~PublicationProfile()
{
}

int
PublicationProfile::interval() const
{
  // Return an exponentially distributed number of microseconds.
  return static_cast<int>(
           1000000.0 *
           -log( static_cast<double>( rand()) / range) / this->rate_
         );
}

int
PublicationProfile::messageSize() const
{
  // Return previously calculated deviate if we have one.
  if( this->sizeAvailable_) {
    this->sizeAvailable_ = false;
    return this->nextSize_;
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
  int value = static_cast<int>(
                ((y * factor) * this->deviation_) + this->mean_
              );

  //  Bounded by the specified maximum and minimum values.
  this->sizeAvailable_ = true;
  this->nextSize_ = std::min( this->max_, std::max( this->min_, value));

  //
  // And return the other of the two generated deviates.
  //

  // Z-adjusted to the desired mean and deviation.
  value = static_cast<int>(
            ((x * factor) * this->deviation_) + this->mean_
          );

  //  Bounded by the specified maximum and minimum values.
  return std::min( this->max_, std::max( this->min_, value));
}

} // End of namespace Test

