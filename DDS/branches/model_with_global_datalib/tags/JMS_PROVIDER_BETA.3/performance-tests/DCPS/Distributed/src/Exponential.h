// -*- C++ -*-
//
// $Id$
#ifndef EXPONENTIAL_H
#define EXPONENTIAL_H

namespace Test {

class Exponential  {
  public:
    /// Default constructor.
    Exponential();

    //@{ @name Interval rate.
    double& rate();
    double  rate() const;
    //@}

    /**
     * @brief Exponentially distributed interval.
     * 
     *  This is an exponential random variate at the interval rate.  This
     *  will generate a sequence of intervals with a Poisson distribution.
     */
    double value() const;

  private:
    /// Rate for which intervals are determined.
    double rate_;
};

inline
Exponential::Exponential()
 : rate_( 0.0)
{
}

inline
double&
Exponential::rate()
{
  return this->rate_;
}

inline
double
Exponential::rate() const
{
  return this->rate_;
}

} // End of namespace Test

#endif // EXPONENTIAL_H

