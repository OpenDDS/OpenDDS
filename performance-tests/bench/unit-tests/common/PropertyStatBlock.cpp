/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "PropertyStatBlock.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

void verify_ssb_derived_values(const Bench::SimpleStatBlock& ssb)
{
  // Assumes median buffer is big enough to hold all samples.
  // Then calculates normal versions of derived values to compare
  // against incremental / consolidation calculations of derived values
  EXPECT_EQ(ssb.median_sample_count_, ssb.sample_count_);
  EXPECT_EQ(ssb.median_sample_overflow_, 0u);
  if (ssb.sample_count_) {
    double min = ssb.median_buffer_[0];
    double max = ssb.median_buffer_[0];
    double sum = 0.0;
    for (size_t i = 0; i < ssb.sample_count_; ++i) {
      const double value = ssb.median_buffer_[i];
      sum += value;
      min = std::min(value, min);
      max = std::max(value, max);
    }
    const double mean = sum / ssb.sample_count_;
    EXPECT_FLOAT_EQ(ssb.mean_, mean);
    EXPECT_FLOAT_EQ(ssb.min_, min);
    EXPECT_FLOAT_EQ(ssb.max_, max);
    double var_x_count = 0.0;
    for (size_t i = 0; i < ssb.sample_count_; ++i) {
      const double value = ssb.median_buffer_[i];
      const double mean_diff = value - mean;
      var_x_count += (mean_diff * mean_diff);
    }
    EXPECT_FLOAT_EQ(ssb.var_x_sample_count_, var_x_count);

    std::vector<double> mb = ssb.median_buffer_;
    mb.resize(ssb.median_sample_count_);
    const size_t mbs = mb.size();

    std::sort(mb.begin(), mb.end());
    const double median = mbs % 2 == 0 ? ((mb[mbs / 2 - 1] + mb[mbs / 2]) / 2.0) : mb[(mbs - 1) / 2];

    EXPECT_FLOAT_EQ(ssb.median_, median);
    for (size_t i = 0; i < ssb.sample_count_; ++i) {
      const double value = ssb.median_buffer_[i];
      mb[i] = std::fabs(value - median);
    }
    std::sort(mb.begin(), mb.end());
    const double mad = mbs % 2 == 0 ? ((mb[mbs / 2 - 1] + mb[mbs / 2]) / 2.0) : mb[(mbs - 1) / 2];
    EXPECT_FLOAT_EQ(ssb.median_absolute_deviation_, mad);
  }
}

TEST(PropertyStatBlock, NominalUsage_OddCount)
{
  Builder::PropertySeq ps;
  Bench::PropertyStatBlock psb(ps, "test", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb.update(1.0);
  psb.update(3.0);
  psb.update(5.0);

  psb.finalize();

  Bench::SimpleStatBlock ssb;
  psb.to_simple_stat_block(ssb);

  verify_ssb_derived_values(ssb);

  EXPECT_EQ(ssb.sample_count_, 3u);
  EXPECT_EQ(ssb.min_, 1.0);
  EXPECT_EQ(ssb.max_, 5.0);

  const double variance = ssb.var_x_sample_count_ / ssb.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb.mean_, 3.0);
  EXPECT_EQ(standard_deviation, std::sqrt(8.0 / 3.0));

  EXPECT_EQ(ssb.median_, 3.0);
  EXPECT_EQ(ssb.median_absolute_deviation_, 2.0);
}

TEST(PropertyStatBlock, NominalUsage_EvenCount)
{
  Builder::PropertySeq ps;
  Bench::PropertyStatBlock psb(ps, "test", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb.update(2.0);
  psb.update(4.0);
  psb.update(6.0);
  psb.update(8.0);

  psb.finalize();

  Bench::SimpleStatBlock ssb;
  psb.to_simple_stat_block(ssb);

  verify_ssb_derived_values(ssb);

  EXPECT_EQ(ssb.sample_count_, 4u);
  EXPECT_EQ(ssb.min_, 2.0);
  EXPECT_EQ(ssb.max_, 8.0);

  const double variance = ssb.var_x_sample_count_ / ssb.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb.mean_, 5.0);
  EXPECT_EQ(standard_deviation, std::sqrt(20.0 / 4.0));

  EXPECT_EQ(ssb.median_, 5.0);
  EXPECT_EQ(ssb.median_absolute_deviation_, 2.0);
}

TEST(PropertyStatBlock, Consolidate)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);
  psb1.update(2.0);
  psb1.update(3.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);
  psb2.update(12.0);
  psb2.update(13.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 6u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 13.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 7.0);
  EXPECT_EQ(standard_deviation, std::sqrt(154.0 / 6.0));

  EXPECT_EQ(ssb3.median_, 7.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, ConsolidateVector)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);
  psb1.update(2.0);
  psb1.update(3.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);
  psb2.update(12.0);
  psb2.update(13.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 6u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 13.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 7.0);
  EXPECT_EQ(standard_deviation, std::sqrt(154.0 / 6.0));

  EXPECT_EQ(ssb3.median_, 7.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, Consolidate_Uneven)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(7.0);
  psb1.update(2.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(3.0);
  psb2.update(4.0);
  psb2.update(4.0);
  psb2.update(5.0);
  psb2.update(6.0);
  psb2.update(1.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 8u);
  EXPECT_FLOAT_EQ(ssb3.min_, 1.0);
  EXPECT_FLOAT_EQ(ssb3.max_, 7.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_FLOAT_EQ(ssb3.mean_, 4.0);
  EXPECT_FLOAT_EQ(standard_deviation, std::sqrt(28.0 / 8.0));

  EXPECT_FLOAT_EQ(ssb3.median_, 4.0);
  EXPECT_FLOAT_EQ(ssb3.median_absolute_deviation_, 1.5);
}

TEST(PropertyStatBlock, ConsolidateVector_Uneven)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(7.0);
  psb1.update(2.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(3.0);
  psb2.update(4.0);
  psb2.update(4.0);
  psb2.update(5.0);
  psb2.update(6.0);
  psb2.update(1.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 8u);
  EXPECT_FLOAT_EQ(ssb3.min_, 1.0);
  EXPECT_FLOAT_EQ(ssb3.max_, 7.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_FLOAT_EQ(ssb3.mean_, 4.0);
  EXPECT_FLOAT_EQ(standard_deviation, std::sqrt(28.0 / 8.0));

  EXPECT_FLOAT_EQ(ssb3.median_, 4.0);
  EXPECT_FLOAT_EQ(ssb3.median_absolute_deviation_, 1.5);
}

TEST(PropertyStatBlock, Consolidate_CountOne)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, ConsolidateVector_CountOne)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, Consolidate_CountZero)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(1.0);
  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, ConsolidateVector_CountZero)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(1.0);
  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, Consolidate_CountZero_OtherDirection)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);
  psb1.update(11.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, ConsolidateVector_CountZero_OtherDirection)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);
  psb1.update(11.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb3);

  EXPECT_EQ(ssb3.sample_count_, 2u);
  EXPECT_EQ(ssb3.min_, 1.0);
  EXPECT_EQ(ssb3.max_, 11.0);

  const double variance = ssb3.var_x_sample_count_ / ssb3.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb3.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb3.median_, 6.0);
  EXPECT_EQ(ssb3.median_absolute_deviation_, 5.0);
}

TEST(PropertyStatBlock, ConsolidateVector_CountZero_Multiple)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  verify_ssb_derived_values(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  verify_ssb_derived_values(ssb2);

  Builder::PropertySeq ps3;
  Bench::PropertyStatBlock psb3(ps3, "test3", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb3.update(1.0);
  psb3.update(11.0);

  psb3.finalize();

  Bench::SimpleStatBlock ssb3;
  psb3.to_simple_stat_block(ssb3);

  verify_ssb_derived_values(ssb3);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);
  ssb_vec.push_back(ssb3);

  const Bench::SimpleStatBlock ssb4 = consolidate(ssb_vec);

  verify_ssb_derived_values(ssb4);

  EXPECT_EQ(ssb4.sample_count_, 2u);
  EXPECT_EQ(ssb4.min_, 1.0);
  EXPECT_EQ(ssb4.max_, 11.0);

  const double variance = ssb4.var_x_sample_count_ / ssb4.sample_count_;
  const double standard_deviation = std::sqrt(variance);

  EXPECT_EQ(ssb4.mean_, 6.0);
  EXPECT_EQ(standard_deviation, std::sqrt(50.0 / 2.0));

  EXPECT_EQ(ssb4.median_, 6.0);
  EXPECT_EQ(ssb4.median_absolute_deviation_, 5.0);
}


}

