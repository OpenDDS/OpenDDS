// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_STATS_T_H
#define OPENDDS_DCPS_STATS_T_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/**
 * @class Stats< DataType>
 *
 * @brief Accumulates average, n, variance, min, and max statistics
 */
template< typename DataType>
class Stats {
  public:
    /// Default constructor.
    Stats();

    /// Default bitwise copy is sufficient.

    /// Assignment operator
    Stats& operator=( const Stats& rhs);

    /// Reset statistics to nil.
    void reset();

    /**
     * Accumulate a new value.
     * @param value the new value to be accumulated.
     */
    void add( DataType value);

    /// Calculate the average value.
    long double mean() const;

    /// Calculate the variance value.
    long double var() const;

    /// Access the minimum value.
    DataType min() const;

    /// Access the maximum value.
    DataType max() const;

    /// Access the number of values accumulated.
    unsigned long n() const;

  private:
    // Direct statistics.
    unsigned long n_;
    DataType      min_;
    DataType      max_;

    // Internal variables have the largest range and highest precision possible.
    long double an_ ;
    long double bn_ ;
    long double cn_ ;
    long double variance_ ;
};

template< typename DataType>
inline
Stats<DataType>::Stats()
{
  this->reset();
}

template< typename DataType>
inline
Stats<DataType>&
Stats<DataType>::operator=( const Stats& rhs)
{
  this->n_        = rhs.n_;
  this->min_      = rhs.min_;
  this->max_      = rhs.max_;
  this->an_       = rhs.an_ ;
  this->bn_       = rhs.bn_ ;
  this->cn_       = rhs.cn_ ;
  this->variance_ = rhs.variance_ ;
  return *this;
}

template< typename DataType>
inline
void
Stats<DataType>::reset()
{
  this->n_        = 0;
  this->min_      = static_cast<DataType>(0);
  this->max_      = static_cast<DataType>(0);
  this->an_       = 0.0;
  this->bn_       = 0.0;
  this->cn_       = 0.0;
  this->variance_ = 0.0;
}

template< typename DataType>
inline
void
Stats<DataType>::add( DataType value)
{
  // Slide rule style calculations.
  long double term;

  //
  // V(N+1) = V(N) * N^2 / (N+1)^2
  //        + A(N)
  //        - B(N) * X(N+1)
  //        + C(N) * X(N+1)^2
  //
  this->variance_ /= (this->n_ + 1);
  this->variance_ *=  this->n_;
  this->variance_ /= (this->n_ + 1);
  this->variance_ *=  this->n_;

  term = static_cast< long double>( value);
  this->variance_ +=  this->an_;
  this->variance_ -=  this->bn_ * term;
  this->variance_ +=  this->cn_ * term * term;

  // The internal variable updates _must_ follow the variance update.

  //
  // A(N+1) = (A(N) * (N+1)^2 / (N+2)^2) + (X(N+1) / (N+2)^2)
  //
  this->an_ /= (this->n_ + 2);
  this->an_ *= (this->n_ + 1);
  this->an_ /= (this->n_ + 2);
  this->an_ *= (this->n_ + 1);

  // term = static_cast< long double>( value);
  term *= term;
  term /= (this->n_ + 2);
  term /= (this->n_ + 2);
  this->an_ += term;

  //
  // B(N+1) = (B(N) * (N+1)^2 / (N+2)^2) + (2 * X(N+1) / (N+2)^2)
  //
  this->bn_ /= (this->n_ + 2);
  this->bn_ *= (this->n_ + 1);
  this->bn_ /= (this->n_ + 2);
  this->bn_ *= (this->n_ + 1);

  term = static_cast< long double>( value * 2);
  term /= (this->n_ + 2);
  term /= (this->n_ + 2);
  this->bn_ += term;

  //
  // C(N+1) = (N+1) / (N+2)^2
  //
  this->cn_  =  this->n_ + 1;
  this->cn_ /= (this->n_ + 2);
  this->cn_ /= (this->n_ + 2);

  if( (this->n_ == 0) || (value < this->min_)) {
    this->min_ = value;
  }

  if( (this->n_ == 0) || (value > this->max_)) {
    this->max_ = value;
  }

  this->n_ += 1; // Must follow internal variable updates.
}

template< typename DataType>
inline
long double
Stats<DataType>::mean() const
{
  if( this->n_ == 0) {
    /// @TODO: return qNaN with no data.
    return 0.0;
  }

  // Slide rule style calculations.

  //
  // MEAN = B(N) * (N+1)^2 / (2 * N)
  //
  long double average = this->bn_ / 2.0 ;

  average *= (this->n_ + 1) ;
  average /=  this->n_ ;
  average *= (this->n_ + 1) ;

  return average ;
}

template< typename DataType>
inline
long double
Stats<DataType>::var() const
{
  return this->variance_ ;
}

template< typename DataType>
inline
DataType
Stats<DataType>::min() const
{
  /// @TODO: return qNaN with no data.
  return (this->n_ == 0)? 0: this->min_;
}

template< typename DataType>
inline
DataType
Stats<DataType>::max() const
{
  /// @TODO: return qNaN with no data.
  return (this->n_ == 0)? 0: this->max_;
}

template< typename DataType>
inline
unsigned long
Stats<DataType>::n() const
{
  return this->n_;
}

#endif // OPENDDS_DCPS_STATS_T_H

