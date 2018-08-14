/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "TreeNode.h"
#include "GvOptions.h"
#include "iostream"
#include "fstream"
#include "stack"

namespace Monitor {

class GraphGenerator {

 public:

  // generate the new graph
  void draw();

  GraphGenerator (TreeNode *, GvOptionsData, bool honorDisplayFlag = false);

 private:
  TreeNode *root_;
  GvOptionsData gvOpt_;
  bool honorDisplayFlag_; // if true only graph treeNodes that have display flag set

  // find readers and writers
  // recursive so it need the TreeNode arg
  void connectNodes();

  void generate(TreeNode *);

  std::string qcolorToHex(const QColor&); // helper for generate
  void htmlEncode(std::string &s); // remove special characters <, >, etc

  std::string abbr(const std::string &);

  std::string nodeName(int offset = 0);

  std::ofstream fout;

  // these are used for finding connections in the graphs
  std::map<std::string, std::string> rMap_; // readers to writers
  std::map<std::string, std::string> wMap_; // writers to readers
  std::map<std::string, std::string> nMap_; // readers and writers to nodeNames

  // when we create a sub graph push "}" so we can pop it later
  std::stack<std::string> lifo_;

};

}; // namespace Monitor

