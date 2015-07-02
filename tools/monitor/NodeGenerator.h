/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QGraphicsScene>
#include <string>

#include "TreeNode.h"
#include "NodeOptions.h"
#include "Node.h"
#include "Edge.h"
#include "iostream"
#include "fstream"
#include "stack"

namespace Monitor {

class NodeGenerator {

 public:

  // calculate the layout based on options and number of nodes
  void calcLayout(TreeNode *, int w, int h);
  int getSceneWidth() { return sceneWidth_; }
  int getSceneHeight() { return sceneHeight_; }

  // generate the new graph
  void draw(QGraphicsScene *);

  NodeGenerator (TreeNode *, NodeOptionsData, bool honorDisplayFlag = false);

private:
  TreeNode *root_;
  NodeOptionsData nodeOpt_;
  int sceneWidth_;
  int sceneHeight_;
  bool honorDisplayFlag_; // if true only graph treeNodes that have display flag set
  std::vector<Edge *> edges_;

  // find readers and writers
  void findConnections(TreeNode *);
  void connectNodes(QGraphicsScene *);

  void generate(TreeNode *, QGraphicsScene *, int w, int h, Node* parent = 0);

  // check to see if node is valid and populate node text
  bool isNodeValid(TreeNode *t, QString &text);

  std::string abbr(const std::string &);

  std::ofstream fout;

  // these are used for finding connections in the graphs
  std::map<TreeNode *, Node *> nMap_;
  std::map<TreeNode *, std::string> rMap_;
  std::map<std::string, TreeNode *> wMap_;

};

}; // namespace Monitor

