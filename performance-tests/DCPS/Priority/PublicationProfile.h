// -*- C++ -*-
//
#ifndef PUBLICATIONPROFILE_H
#define PUBLICATIONPROFILE_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <string>

namespace Test {

class PublicationProfile  {
  public:
    /**
     * @brief Construct with publication parameters.
     *
     * @param name      - identification for this publication.
     * @param priority  - message priority for this publication.
     * @param rate      - messages per second which will be generated.
     * @param size      - average bytes of generated messages.
     * @param deviation - standard deviation of message sizes.
     */
    PublicationProfile(
      const std::string& name,
      int priority  = 0,
      int rate      = 100,
      int size      = 1000,
      int deviation = 300,
      int max       = 1450,
      int min       = 800
    );

    virtual ~PublicationProfile();

    /// This publications identification.
    const std::string& name() const;

    /// This publications priority.
    int priority() const;

    /**
     * @brief Message size for the next message.
     *
     *  This is a Gaussian distribution of message sizes.  These sizes
     *  are clamped to the specified maximum and minimum values.  This
     *  action will result in all of the deviates generated beyond these
     *  points being set to the extremum values - which distorts the
     *  ends of the distribution.
     */
    int messageSize() const;

    /**
     * @brief Interval in microseconds until next message.
     *
     *  This is an exponential random variate at the interval rate.  This
     *  will generate a stream of intervals with a Poisson distribution.
     */
    int interval() const;

  private:
    /// Identification of the publication.
    std::string name_;

    /// Value of the TRANSPORT_PRIORITY policy for this publication.
    int priority_;

    //
    // These are stored in the more useful double format.
    //

    /// Rate at which messages are generated.
    double rate_;

    /// Average message size.
    double mean_;

    /// Standard deviation of generated message sizes.
    double deviation_;

    //
    // These are not *profile* state but value generation details.
    //

    /// Flag indicating a previously generated size is available.
    mutable int nextSize_;

    /// A previously generated random size value.
    mutable bool sizeAvailable_;

    //
    // Limits on message size.
    //

    /// Largest message size allowed.
    int max_;

    /// Smallest message size allowed.
    int min_;
};

inline
const std::string&
PublicationProfile::name() const
{
  return this->name_;
}

inline
int
PublicationProfile::priority() const
{
  return this->priority_;
}

} // End of namespace Test

#endif // PUBLICATIONPROFILE_H

