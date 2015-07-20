// ============================================================================
// -*- C++ -*-
// ============================================================================
/**
 *  @file   SampleInfo.cpp
 *
 *
 *
 */
// ============================================================================

#include "SampleInfo.h"
#include "ace/OS_NS_stdio.h"

#include <string>

/*
 * Print sample info
 */
void PrintSampleInfo(const ::DDS::SampleInfo& si)
{
  std::string out;

  switch (si.sample_state)
  {
    case ::DDS::READ_SAMPLE_STATE:
      out = "READ_SAMPLE_STATE";
      break;

    case ::DDS::NOT_READ_SAMPLE_STATE:
      out = "NOT_READ_SAMPLE_STATE";
      break;
  }
  out = "  sample_state: " + out + "\n";

  std::string tmp;

  switch(si.view_state)
  {
    case ::DDS::NEW_VIEW_STATE:
      tmp = "NEW_VIEW_STATE";
      break;

    case ::DDS::NOT_NEW_VIEW_STATE:
      tmp = "NOT_NEW_VIEW_STATE";
      break;
  }
  out += "  view_state: " + tmp + "\n";

  tmp = "";

  switch(si.instance_state)
  {
    case ::DDS::ALIVE_INSTANCE_STATE:
      tmp = "ALIVE_INSTANCE_STATE";
      break;

    case ::DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
      tmp = "NOT_ALIVE_DISPOSED_INSTANCE_STATE";
      break;

    case ::DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      tmp = "NOT_ALIVE_NO_WRITERS_INSTANCE_STATE";
      break;
  }
  out += "  instance_state: " + tmp + "\n";

  char buf[512];

  ACE_OS::snprintf(buf, sizeof buf, "  source_timestamp: %d %d\n",
                       si.source_timestamp.sec, si.source_timestamp.nanosec);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  instance_handle: %x\n", si.instance_handle);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  disposed_generation_count: %d\n",
                  si.disposed_generation_count);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  no_writers_generation_count: %d\n",
                  si.no_writers_generation_count);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  sample_rank: %d\n", si.sample_rank);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  generation_rank: %d\n", si.generation_rank);
  out += buf;

  ACE_OS::snprintf(buf, sizeof buf, "  absolute_generation_rank: %d\n",
                  si.absolute_generation_rank);
  out += buf;

  ACE_OS::fprintf (stderr, "%s\n", out.c_str());
}


bool SampleInfoMatches(const ::DDS::SampleInfo& si1, const ::DDS::SampleInfo& si2)
{
  if (si1.sample_state != si2.sample_state)
  {
    ACE_OS::fprintf (stderr, "sample_state is different\n");
    return false;
  }

  if (si1.view_state != si2.view_state)
  {
    ACE_OS::fprintf (stderr, "view_state is different\n");
    return false;
  }

  if (si1.instance_state != si2.instance_state)
  {
    ACE_OS::fprintf (stderr, "instance_state is different\n");
    return false;
  }

  if (si1.disposed_generation_count != si2.disposed_generation_count)
  {
    ACE_OS::fprintf (stderr, "disposed_generation_count is different\n");
    return false;
  }

  if (si1.no_writers_generation_count != si2.no_writers_generation_count)
  {
    ACE_OS::fprintf (stderr, "no_writers_generation_count is different\n");
    return false;
  }

  if (si1.sample_rank != si2.sample_rank)
  {
    ACE_OS::printf("sample_rank is different\n");
    return false;
  }

  if (si1.generation_rank != si2.generation_rank)
  {
    ACE_OS::fprintf (stderr, "generation_rank is different\n");
    return false;
  }

  if (si1.absolute_generation_rank != si2.absolute_generation_rank)
  {
    ACE_OS::fprintf (stderr, "generation_rank is different\n");
    return false;
  }

  return true;
}
