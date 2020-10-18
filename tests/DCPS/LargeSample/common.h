#ifndef LARGE_SAMPLE_COMMON_H
#define LARGE_SAMPLE_COMMON_H

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsInfrastructureC.h>

const DDS::DomainId_t domain = 113;
const size_t default_writer_process_count = 2;
const size_t default_writers_per_process = 2;
const size_t default_samples_per_writer = 10;
const size_t default_data_field_length_offset = 0;

unsigned expected_data_field_length(unsigned offset, int writer_id, int sample_id);
unsigned char expected_data_field_element(int writer_id, int sample_id, int j);
#ifdef OPENDDS_SECURITY
void append(
  DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false);
void set_security_qos(DDS::DomainParticipantQos& participant_qos, unsigned secid);
#endif

#endif // LARGE_SAMPLE_COMMON_H
