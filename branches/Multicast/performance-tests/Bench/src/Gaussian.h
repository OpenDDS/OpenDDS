// -*- C++ -*-
//
// $Id$
#ifndef GAUSSIAN_H
#define GAUSSIAN_H

namespace Test {

class Gaussian  {
  public:
    /// Default constructor.
    Gaussian();

    /// @name Mean value.
    /// @{
    double& mean();
    double  mean() const;
    ///@}

    /// @name Standard deviation.
    /// @{
    double& deviation();
    double  deviation() const;
    /// @}

    /// @name Upper bound.
    /// @{
    double& maximum();
    double  maximum() const;
    /// @}

    /// @name Lower bound.
    /// @{
    double& minimum();
    double  minimum() const;
    /// @}

    /**
     * @brief Next gaussian distributed value.
     * 
     *  This is a random Gaussian variate.  The value is clamped to the
     *  specified maximum and minimum values.  This action will result in
     *  all of the deviates generated beyond these points being set to the
     *  extremum values - which distorts the ends of the distribution.
     */
    double value() const;

  private:
    /// Mean.
    double mean_;

    /// Standard deviation.
    double deviation_;

    //
    // These are value generation details.
    //

    /// Flag indicating a previously generated size is available.
    mutable double nextValue_;

    /// A previously generated random value.
    mutable bool valueAvailable_;

    //
    // Limits on value.
    //

    /// Largest value allowed.
    double max_;

    /// Smallest value allowed.
    double min_;
};

inline
Gaussian::Gaussian()
 : mean_( 0.0),
   deviation_( 1.0),
   nextValue_( 0.0),
   valueAvailable_( false),
   max_( 0.0),
   min_( 0.0)
{
}

inline
double&
Gaussian::mean()
{
  this->valueAvailable_ = false;
  return this->mean_;
}

inline
double
Gaussian::mean() const
{
  return this->mean_;
}

inline
double&
Gaussian::deviation()
{
  this->valueAvailable_ = false;
  return this->deviation_;
}

inline
double
Gaussian::deviation() const
{
  return this->deviation_;
}

inline
double&
Gaussian::maximum()
{
  this->valueAvailable_ = false;
  return this->max_;
}

inline
double
Gaussian::maximum() const
{
  return this->max_;
}

inline
double&
Gaussian::minimum()
{
  this->valueAvailable_ = false;
  return this->min_;
}

inline
double
Gaussian::minimum() const
{
  return this->min_;
}

} // End of namespace Test

#endif // GAUSSIAN_H

