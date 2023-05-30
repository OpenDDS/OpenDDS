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

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);
  psb2.update(12.0);
  psb2.update(13.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

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

TEST(PropertyStatBlock, Consolidate_CountOne)
{
  Builder::PropertySeq ps1;
  Bench::PropertyStatBlock psb1(ps1, "test1", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb1.update(1.0);

  psb1.finalize();

  Bench::SimpleStatBlock ssb1;
  psb1.to_simple_stat_block(ssb1);

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb1, ssb2);

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

  Builder::PropertySeq ps2;
  Bench::PropertyStatBlock psb2(ps2, "test2", Bench::DEFAULT_STAT_BLOCK_BUFFER_SIZE);

  psb2.update(11.0);

  psb2.finalize();

  Bench::SimpleStatBlock ssb2;
  psb2.to_simple_stat_block(ssb2);

  std::vector<Bench::SimpleStatBlock> ssb_vec;
  ssb_vec.push_back(ssb1);
  ssb_vec.push_back(ssb2);

  const Bench::SimpleStatBlock ssb3 = consolidate(ssb_vec);

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

}

