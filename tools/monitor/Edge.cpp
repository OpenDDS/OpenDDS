/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "Edge.h"

Monitor::Edge::Edge(Node* ancestor, Node* child, bool pubSub, QGraphicsItem *):
parent_(ancestor),
child_(child),
pubSub_(pubSub)
{
  setFlag(ItemIsMovable);
//  setFlag(ItemIsSelectable);
  setFlag(ItemSendsGeometryChanges);
  setCacheMode(DeviceCoordinateCache);
  setZValue(1);
}

void
Monitor::Edge::adjust()
{
  prepareGeometryChange();
}

QRectF
Monitor::Edge::boundingRect() const
{
  if (!parent_ || !child_) {
    return QRectF();
  }

  int x1 = parent_->scenePos().x() + parent_->boundingRect().width() - 2;
  int y1 = parent_->scenePos().y() + parent_->boundingRect().height() - 2;
  int x2 = child_->scenePos().x() + 2;
  int y2 = child_->scenePos().y() + 2;

  // calculate the proper bounding rect.
  if (x1 < x2) {
    if (y1 < y2) {
      return QRectF(x1, y1, x2, y2);
    }
    else {
      return QRectF(x1, y2, x2, y1);
    }
  }
  else {
    if (y1 < y2) {
      return QRectF(x2, y1, x1, y2);
    }
    else {
      return QRectF(x2, y2, x1, y1);
    }
  }

}

void
Monitor::Edge::paint(QPainter *painter,const QStyleOptionGraphicsItem *,QWidget *)
{
  if (pubSub_) {
    QPen pen;
    pen.setWidth(3);
    painter->setPen(pen);
  }

  int x1 = parent_->scenePos().x() + parent_->boundingRect().width() - 2;
  int y1 = parent_->scenePos().y() + parent_->boundingRect().height() - 2;
  int x2 = child_->scenePos().x() + 2;
  int y2 = child_->scenePos().y() + 2;

  QLine line;
  line.setLine(x1, y1, x2, y2);

  painter->drawLine(line);

  update();
}

