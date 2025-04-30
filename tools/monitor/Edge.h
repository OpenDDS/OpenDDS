/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef EDGE_H
#define EDGE_H

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QPainter>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

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
