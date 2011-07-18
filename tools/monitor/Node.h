/*
* $Id$
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QGraphicsTextItem>
#include <QtGui/QPainter>

#include "TreeNode.h"

#ifndef NODE_H
#define NODE_H

#define NODE_EDIT "Edit"
#define NODE_DELETE "Delete"

namespace Monitor {

class Node : public QGraphicsTextItem
{
public:
	Node (QString& text, qreal x, qreal y, TreeNode *t, QGraphicsItem * parent = 0);
		
	// override
	void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget);

protected:	
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	
private:
	// connection info
	std::vector<Node *>parents_;
	std::vector<Node *>children_;
	
	// the model
  TreeNode* treeNode_;  
  
};

};
#endif /* NODE_H */
