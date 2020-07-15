/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SECURITY_QOS_TEST_ARGS_H
#define SECURITY_QOS_TEST_ARGS_H

#include "dds/DdsDcpsCoreC.h"

#include "ace/ace_wchar.h"

#include <string>
#include <vector>

namespace SecurityAttributes
{

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
  bool wait_for_acks_;

  int num_messages_;

  int expected_result_;
  int timeout_;

  int extra_space_;

  Args();

  void partition_to_qos(DDS::PartitionQosPolicy& policy);

  static int parse_args(int argc, ACE_TCHAR *argv[], Args& args);
};

} // namespace SecurityAttributes


#endif
