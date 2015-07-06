// -*- C++ -*-
//
#ifndef STATISTICALVALUE_H
#define STATISTICALVALUE_H

/**
 * @file StatisticalValue.h
 *
 * This file contains several class declarations with template method
 * implementations.  The combination of these classes and their
 * implementations provide a simple mechanism for allowing the use of
 * statistical deviate values in the calling code.
 *
 * The classes are:
 *   StatisticalValue<Type> - abstract interface base class.
 *   StatisticalImpl        - internal delegate class with implementations.
 *   FixedValue<Type>       - fixed values using the StatisticalValue idiom.
 *   UniformValue<Type>     - uniform distribution deviate value.
 *   ExponentialValue<Type> - exponential distribution deviate value.
 *   GaussianValue<Type>    - Gaussian distribution deviate value.
 */

namespace Test {

/**
 * class StatisticalValue<Type>
 *
 * @brief Abstract base class for accessing statistical values.
 *
 * This class provides the abstract interface for accessing statistical
 * values from different types of distributions.  It provides enough
 * methods for all supported distributions, and where a method is not
 * required for a distribution, it is ignored during processing.
 *
 * The interfaces methods are provided via a template parameter type.
 * The "maximum" and "minimum" values are used to constrain the returned
 * value to be within this range.  The implementation of a distribution
 * will check for out of range values and replace them with the closest
 * extreme value.
 *
 * Values are accessed by type conversion to the template parameter
 * specified type.  So each assignment of object of this class to
 * variables of the template parameter type will result in a new deviate
 * value being generated and assigned.
 */
template< class Type>
class StatisticalValue {
  public:
    StatisticalValue();

    virtual ~StatisticalValue();

    /// Create a new value of Type.
    virtual operator Type() const = 0;

    /// @name Upper bound.
    /// @{
    Type& maximum();
    Type  maximum() const;
    /// @}

    /// @name Lower bound.
    /// @{
    Type& minimum();
    Type  minimum() const;
    /// @}

    /// @name Mean value.
    /// @{
    Type& mean();
    Type  mean() const;
    /// @}

    /// @name Standard deviation, if meaningful.
    /// @{
    double& deviation();
    double  deviation() const;
    /// @}

  protected:
    Type max_;            // Upper bound for values.
    Type min_;            // Lower bound for values.
    Type mean_;           // Average (in some sense) for values.
    double deviation_;    // Standard deviation, if meaningful.
    mutable bool dirty_;  // Flag to indicate calculations are required.
};

/**
 * @class StatisticalImpl
 *
 * @brief Delegate to implement actual statistical value generation.
 *
 * This class provides the individual statistical deviate generation
 * methods to be used by the specific template classes.  This allows the
 * generation methods to be isolated from the dependencies and template
 * instantiation requirements of the interface structures.
 */
class StatisticalImpl {
  public:
    StatisticalImpl();

    /// Extract a deviate value from a uniform distribution.
    double uniform(
             double lower,
             double upper
           ) const;

    /// Extract a deviate value from an exponential distribution.
    double exponential( double mean) const;

    /// Extract a deviate value from a Gaussian distribution.
    double gaussian(
             bool&  dirty,
             double mean,
             double lower,
             double upper,
             double deviation
           ) const;

  private:
    /// Cached deviate value used by the gaussian() method.  The
    /// Box-Muller transformation results in generating pairs of
    /// values, so we cache one here for use on the next call.
    mutable double nextValue_;
};

/**
 * @class FixedValue<Type>
 *
 * @brief Access fixed values via a statistical variable interface.
 *
 * This class provides an implementation of the StatisticalValue<Type>
 * abstract interface that allows fixed single values to be accessed via
 * the statistical distribution methods.  This allows variables that
 * expect a statistical deviate value to be provided with a fixe value.
 *
 * Only the "mean" value has meaning for this pseudo-distribution.  This
 * is where the value is dervied from.
 */
template< class Type>
class FixedValue : public StatisticalValue<Type>  {
  public:
    /// Return the value.
    virtual operator Type() const;
};

/**
 * @class UniformValue<Type>
 *
 * @brief Access values from a uniform distribution.
 *
 * This class provides access to statistical deviate values derived from
 * a uniform distribution.
 *
 * Only the "minimum" and "maximum" values have meaning for this
 * distribution.  The value returned will be uniformly distributed
 * between these two extremes.
 */
template< class Type>
class UniformValue : public StatisticalValue<Type>  {
  public:
    /// Generate a new Uniform deviate value.
    virtual operator Type() const;

  private:
    /// Delegate for deviate generation.
    StatisticalImpl impl_;
};

/**
 * @class ExponentialValue<Type>
 *
 * @brief Access values from a Exponential distribution.
 *
 * This class provides access to statistical deviate values derived from
 * an Exponential distribution.  If used for an interval value, this can
 * be used to create Poisson arrival times.
 *
 * Only the "mean" value has meaning for this distribution.  This value
 * is used for the average value of the Exponential distribution, which
 * has only this parameter.
 */
template< class Type>
class ExponentialValue : public StatisticalValue<Type>  {
  public:
    /// Generate a new Exponential deviate.
    virtual operator Type() const;

  private:
    /// Delegate for deviate generation.
    StatisticalImpl impl_;
};

/**
 * @class GaussianValue<Type>
 *
 * @brief Access values from a Gaussian distribution.
 *
 * This class provides access to statistical deviate values derived from
 * a Gaussian distribution.
 *
 * All of the parameters are used by this distribution.  They have the
 * standard statistical meanings ascribed to them.
 */
template< class Type>
class GaussianValue : public StatisticalValue<Type>  {
  public:
    /// Generate a new Gaussian deviate.
    virtual operator Type() const;

  private:
    /// Delegate for deviate generation.
    StatisticalImpl impl_;
};

template< class Type>
inline
StatisticalValue<Type>::StatisticalValue()
 : max_(0), min_(0), mean_(0), deviation_(0.0), dirty_(true)
{
}

template< class Type>
StatisticalValue<Type>::~StatisticalValue()
{
}

template< class Type>
inline
Type&
StatisticalValue<Type>::maximum()
{
  this->dirty_ = true;
  return this->max_;
}

template< class Type>
inline
Type
StatisticalValue<Type>::maximum() const
{
  return this->max_;
}

template< class Type>
inline
Type&
StatisticalValue<Type>::minimum()
{
  this->dirty_ = true;
  return this->min_;
}

template< class Type>
inline
Type
StatisticalValue<Type>::minimum() const
{
  return this->min_;
}

template< class Type>
inline
Type&
StatisticalValue<Type>::mean()
{
  this->dirty_ = true;
  return this->mean_;
}

template< class Type>
inline
Type
StatisticalValue<Type>::mean() const
{
  return this->mean_;
}

template< class Type>
inline
double&
StatisticalValue<Type>::deviation()
{
  this->dirty_ = true;
  return this->deviation_;
}

template< class Type>
inline
double
StatisticalValue<Type>::deviation() const
{
  return this->deviation_;
}

template< class Type>
inline
FixedValue<Type>::operator Type() const
{
  return this->mean_;
}

template< class Type>
inline
UniformValue<Type>::operator Type() const
{
#if 0
  /// @TODO This is a 'const' method, so we can't adjust here, so we
  //        still need to determine where we should do bounds checking.

  // Manage out-of-bounds conditions.
  if( this->dirty_) {
    if( this->max_ < this->min_) {
      this->max_ = this->min_;
    }
    this->dirty_ = false;
  }
#endif

  /// @NOTE: the rounding adjustment works for integer Type
  ///        specifications, but fails for floating point Types.  Our use
  ///        cases are exclusively integer types, so this is ok here.
  return static_cast<Type>(
           0.5 + this->impl_.uniform(
                   static_cast<double>(this->min_),
                   static_cast<double>(this->max_)
                 )
         );
}

template< class Type>
inline
ExponentialValue<Type>::operator Type() const
{
  return static_cast<Type>(
           this->impl_.exponential( static_cast<double>(this->mean_))
         );
}

template< class Type>
inline
GaussianValue<Type>::operator Type() const
{
  return static_cast<Type>(
           this->impl_.gaussian(
             this->dirty_,
             static_cast<double>(this->mean_),
             static_cast<double>(this->min_),
             static_cast<double>(this->max_),
             this->deviation_
           )
         );
}

} // End of namespace Test

#endif // STATISTICALVALUE_H

