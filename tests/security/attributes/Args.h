/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SECURITY_QOS_TEST_ARGS_H
#define SECURITY_QOS_TEST_ARGS_H

#include <dds/DdsDcpsCoreC.h>

#include <ace/ace_wchar.h>

#include <string>
#include <vector>

const std::string part_user_data_string = "SECRET SECRET SECRET";

struct Args {

  static const int DEFAULT_NUM_MESSAGES = 10;

  std::string auth_ca_file_;
  std::string perm_ca_file_;
  std::string id_cert_file_;
  std::string id_key_file_;
  std::string governance_file_;
  std::string permissions_file_;

  int domain_;

  std::string topic_name_;
  std::vector<std::string> partition_;

  bool reliable_;

  int num_messages_;

  int expected_result_;
  int timeout_;

  int extra_space_;

  bool secure_part_user_data_;
  bool expect_part_user_data_;
  bool expect_blank_part_user_data_;

  Args();

  void partition_to_qos(DDS::PartitionQosPolicy& policy);

  static int parse_args(int argc, ACE_TCHAR *argv[], Args& args);
};

#endif
