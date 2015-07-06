/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef EDGE_H
#define EDGE_H

#include <QtGui/QGraphicsTextItem>
#include <QtGui/QPainter>

#include "Node.h"

namespace Monitor {

class Edge : public QGraphicsItem
{
public:
  Edge (Node* ancestor, Node *child, bool pubSub, QGraphicsItem * parent = 0);

  void adjust();

protected:
  QRectF boundingRect() const;

  // override
  void paint(QPainter *painter,const QStyleOptionGraphicsItem *,QWidget *);

private:
  // connection info
  Node* parent_;
  Node* child_;

  // is this a pub/sub connection
  bool pubSub_;

};

};

#endif /* NODE_H */
