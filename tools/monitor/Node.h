/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QGraphicsTextItem>
#include <QtGui/QPainter>
#include <vector>

#include "TreeNode.h"

#ifndef NODE_H
#define NODE_H

namespace Monitor {

class Edge;

class Node : public QGraphicsTextItem
{
public:
  Node (QString& text, qreal x, qreal y, TreeNode *t, QGraphicsItem * parent = 0);

  void addEdge(Edge* edge);

  // override
  void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget);

protected:
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:

  void updateEdges();

  std::vector<Edge *> edges_;

  // the model
  TreeNode* treeNode_;

};

};
#endif /* NODE_H */
