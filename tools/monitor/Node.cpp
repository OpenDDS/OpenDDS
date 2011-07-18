/*
* $Id$
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QMenu>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <cstdio>

#include "Node.h"

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
  setZValue(1);
  
}

void 
Monitor::Node::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{               
	// call the base class to draw the text.
	this->QGraphicsTextItem::paint(painter, option, widget);
	
	// put a border around the node
  painter->drawRect (boundingRect().adjusted(2, 2, -2, -2));
  update();
}


void 
Monitor::Node::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
 {
     QMenu menu;
     QAction *removeAction = menu.addAction("Remove");
     QAction *markAction = menu.addAction("Mark");
     //QAction *selectedAction = menu.exec(event->screenPos());
     // ...
     //printf ("got node context\n");
 }

