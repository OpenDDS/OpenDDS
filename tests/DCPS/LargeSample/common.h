#ifndef LARGE_SAMPLE_COMMON_H
#define LARGE_SAMPLE_COMMON_H

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsInfrastructureC.h>

const DDS::DomainId_t domain = 113;
const size_t default_writer_process_count = 2;
const size_t default_writers_per_process = 2;
const size_t default_samples_per_writer = 10;
const size_t default_data_field_length_offset = 0;

inline unsigned expected_data_field_length(unsigned offset, int writer_id, int sample_id)
{
  // Writer ID is 1 or 2
  // Sample ID is 0 to 9
  // Lengths will vary from 10k to 155k
  return offset + unsigned((sample_id * 1.5) + writer_id) * 10 * 1024;
}

inline unsigned char expected_data_field_element(int writer_id, int sample_id, int j)
{
  return static_cast<unsigned char>(j % 256) + writer_id + sample_id * 3;
}

#ifdef OPENDDS_SECURITY
inline void append(
  DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}
#endif

#endif // LARGE_SAMPLE_COMMON_H
