/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <sstream>

#include "NodeGenerator.h"
#include "Node.h"
#include "TreeNode.h"

// Note calls to QObject::toStdString() have been replaced with QObject::toLocal8Bit().constData()
// to avoid QString-related aborts under windows.

/// Constructor
Monitor::NodeGenerator::NodeGenerator(TreeNode *root, NodeOptionsData nodeOpt,
  bool honorDisplayFlag) :
root_(root)
, nodeOpt_(nodeOpt)
, sceneWidth_(800)
, sceneHeight_(800)
, honorDisplayFlag_(honorDisplayFlag)
{
}

void
Monitor::NodeGenerator::draw(QGraphicsScene *nodeScene)
{
  nMap_.clear();
  generate(root_, nodeScene, 0, 0);

  connectNodes(nodeScene);

  // reset scene size. this doesn't happen automatically
  nodeScene->setSceneRect(nodeScene->itemsBoundingRect());

}

void
Monitor::NodeGenerator::calcLayout(TreeNode *t, int w, int h)
{
  static int xpos = 0;
  static int ypos = 0;

  // reset width and height of scene
  if (t == 0) {
    return;
  }

  QString text;

  if (isNodeValid(t, text)) {

    xpos = w * 100;
    ypos += 50;

    // create nodes to figure out width and height of layout
    Node node(text, xpos, ypos, t);
    sceneWidth_ = xpos + node.boundingRect().width();
    sceneHeight_ = ypos + node.boundingRect().width();

    ++w;
    for (int i = 0; i < t->size(); ++i) {
      calcLayout(t->operator[](i), w, ++h);
    }
  }
}

void
Monitor::NodeGenerator::findConnections(TreeNode * t)
{
  if (t == 0) {
    return;
  }

  std::string key = t->column(0).toString().toLocal8Bit().constData();
  std::string val = t->column(1).toString().toLocal8Bit().constData();

  // store this writer-->reader connection info
  TreeNode *c;
  std::string tmp;
  std::string conn;

  if (key == "Reader" || key == "Writer") {
    for (int i = 0; i < t->size(); ++i) {
      c = t->operator[](i);
      tmp = c->column(0).toString().toLocal8Bit().constData();

      if (nodeOpt_.abbrGUIDs()) {
        conn = abbr(c->column(1).toString().toLocal8Bit().constData());
        val = abbr(t->column(1).toString().toLocal8Bit().constData());
      }
      else {
        conn = c->column(1).toString().toLocal8Bit().constData();
      }

      if (tmp == "Writer" && key == "Reader") {
        rMap_[t] = conn;
      }
      else if (tmp == "Reader" && key == "Writer") {
        wMap_[val] = t;
      }
    }
  }

  for (int i = 0; i < t->size(); ++i) {
    findConnections(t->operator[](i));
  }
}

void
Monitor::NodeGenerator::connectNodes(QGraphicsScene *nodeScene)
{
  // erase old values
  rMap_.clear();
  wMap_.clear();

  findConnections(root_);

  std::map <TreeNode *, std::string>::const_iterator i;

  // connect reader to writers
  for (i = rMap_.begin(); i != rMap_.end(); ++i) {
    Node *parent = nMap_[i->first];
    Node *child = nMap_[wMap_[rMap_[i->first]]];
    if (parent && child) {
      Edge *edge = new Edge(parent, child, true, parent);
      nodeScene->addItem(edge);
      parent->addEdge(edge);
      child->addEdge(edge);
    }
  }
}

void
Monitor::NodeGenerator::generate(TreeNode *t, QGraphicsScene *nodeScene, int w, int h, Node* parent)
{
  static int xpos = 0;
  static int ypos = 0;

  if (t == 0) {
    return;
  }

  Node *node = 0;

  QString text;
  if (isNodeValid(t, text)) {

    xpos = w * 100;
    ypos += 50;

    node = new Node(text, xpos, ypos, t);
    node->setParent(parent);

    // populate tooltips with the children
    TreeNode *c;
    std::ostringstream ttip;
    for (int i = 0; i < t->size(); ++i) {
      c = t->operator[](i);
      std::string k = c->column(0).toString().toLocal8Bit().constData();
      std::string v = c->column(1).toString().toLocal8Bit().constData();

      ttip << k << " : " << v << std::endl;
    }

    node->setToolTip(ttip.str().c_str());

    // this takes ownership of the node so the nodeScene will
    // cleanup all nodes when it's destroyed
    // TODO: This make each nodes parent the nodeScene. change this
    // so parent is the actual parent node using setParent()
    nodeScene->addItem(node);

    // connect us to our parent with a dashed line
    // connect us to the parent
    if (parent  && !nodeOpt_.hideParentChild()) {
      // give edge an actual parent so that z-order works
      Edge *edge = new Edge(parent, node, false,  parent);
      nodeScene->addItem(edge);
      parent->addEdge(edge);
      node->addEdge(edge);
    }
    // store node
    if (parent && !nodeOpt_.hidePubSub()) {
      nMap_[t] = node;
    }
  }

  // draw the child nodes
  ++w;
  for (int i = 0; i < t->size(); ++i) {
    generate(t->operator[](i), nodeScene, w, ++h, node);
  }

}

bool
Monitor::NodeGenerator::isNodeValid(TreeNode *t, QString &text)
{
  bool ret = false;

  // draw this node ?
  if (t != root_ && t->width() >= 2) {
    if (!honorDisplayFlag_ || (honorDisplayFlag_ && t->display())) {
      std::string key = t->column(0).toString().toLocal8Bit().constData();
      std::string val = t->column(1).toString().toLocal8Bit().constData();

      // host and process nodes
      // TODO: this is terribly ugly
      if (
        (key == "Host" && !nodeOpt_.ignoreHosts()) ||
        (key == "Process" && !nodeOpt_.ignoreProcs()) ||
        (key == "DomainParticipant") ||
        (key == "Publisher" && !nodeOpt_.ignorePubs()) ||
        (key == "Subscriber" && !nodeOpt_.ignoreSubs()) ||
        (key == "Topic" && !nodeOpt_.hideTopics()) ||
        (key == "Transport" && !nodeOpt_.ignoreTransports()) ||
        (key == "Reader" || key == "Writer") ||
        (key == "Qos" && !nodeOpt_.ignoreQos())
        ) {

          ret = true;

          // check for ignoreBuiltInTopics
          if (key == "Topic" && !nodeOpt_.hideTopics() &&
              nodeOpt_.ignoreBuiltinTopics()) {

            ret = false;
            std::string tmp;
            TreeNode* c;

            for (int i = 0; i < t->size(); ++i) {
              c = t->operator[](i);
              tmp = c->column(0).toString().toLocal8Bit().constData();

              if (tmp == "Topic Name") {
                tmp = c->column(1).toString().toLocal8Bit().constData();
                if(tmp.substr(0, 4) != "DCPS") {
                  ret = true;
                }
              }
            }
          }

          if (ret) {
            // yes, draw this node!
            if (nodeOpt_.abbrGUIDs()) {
              val = abbr(val);
            }

            text = key.c_str();
            text += "\n";
            text += val.c_str();
          }
        }
    }
  }

  return ret;
}

std::string
Monitor::NodeGenerator::abbr(const std::string &s)
{
  std::string ret = s;

  const char *t = s.c_str();
  while (*t++) {
    if (*t == '(') {
      ret = t;
      break;
    }
  }

  return ret;
}


