/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QMenu>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <vector>

#include "Node.h"
#include "Edge.h"
#include "NodeGenerator.h"

Monitor::Node::Node(QString& text, qreal x, qreal y, TreeNode *t, QGraphicsItem * parent):
QGraphicsTextItem(text, parent),
treeNode_(t)
{
  setX(x);
  setY(y);

  setFlag(ItemIsMovable);
  setFlag(ItemIsSelectable);
  setFlag(ItemSendsGeometryChanges);
  setCacheMode(DeviceCoordinateCache);
  setZValue(2);
}

void
Monitor::Node::addEdge(Edge *edge)
{
  edges_.push_back(edge);
}

void
Monitor::Node::updateEdges()
{
  for (unsigned int i = 0; i < edges_.size(); ++i) {
    edges_[i]->adjust();
    edges_[i]->update();
  }
}

void
Monitor::Node::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{
  // set the node color
  QPen pen;
  pen.setColor(treeNode_->color(1).value<QColor>());
  painter->setPen(pen);

  // call the base class to draw the text.
  this->QGraphicsTextItem::paint(painter, option, widget);

  // put a border around the node
  painter->drawRect (boundingRect().adjusted(2, 2, -2, -2));

}

void
Monitor::Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  update();
  updateEdges();
  QGraphicsItem::mouseReleaseEvent(event);
}


