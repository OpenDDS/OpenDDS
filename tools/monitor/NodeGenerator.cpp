/*
* $Id$
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <sstream>

#include "NodeGenerator.h"
#include "Node.h"
#include "TreeNode.h"
#include "gvc.h" 

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
	generate(root_, nodeScene, 0, 0);

	connectNodes();
	
	// reset scene size. this doesn't happen automatically
	nodeScene->setSceneRect(nodeScene->itemsBoundingRect());
  
}

void
Monitor::NodeGenerator::calcLayout(TreeNode *t, int w, int h)
{
	// reset width and height of scene
	if (t == 0) {
		return;
	}
	
	// Todo: fix this layout
	sceneWidth_ = w * 100;
	sceneHeight_ = h * 50;
	
	++w;
  for (int i = 0; i < t->size(); ++i) {
  	calcLayout(t->operator[](i), w, ++h);
  }
  
  
  
}

void
Monitor::NodeGenerator::connectNodes() 
{ 
}

void
Monitor::NodeGenerator::generate(TreeNode *t, QGraphicsScene *nodeScene, int w, int h)
{
	static int xpos;
	static int ypos;
	
	if (t == 0) {
		return;
	}

	xpos = w * 100;
	ypos = h * 50;
	
	// draw this node
	if (t != root_ && t->width() >= 2) {
		if (!honorDisplayFlag_ || (honorDisplayFlag_ && t->display())) {
  	std::string key = t->column(0).toString().toStdString();   
  	std::string val = t->column(1).toString().toStdString();
  	
  	// host and process nodes
  	// TODO: this is terribly ugly
  	if (
  		(key == "Host" && !nodeOpt_.ignoreHosts()) ||
  		(key == "Process" && !nodeOpt_.ignoreProcs()) ||
  		(key == "DomainParticipant") ||
  		(key == "Publisher" && !nodeOpt_.ignorePubs()) || 
  		(key == "Subscriber" && !nodeOpt_.ignoreSubs()) ||
  		( (key == "Topic" && !nodeOpt_.hideTopics()) &&
  			!(nodeOpt_.ignoreBuiltinTopics() && 
  				val.substr(0, 4) != "DCPS") ) ||
  		(key == "Transport" && !nodeOpt_.ignoreTransports()) ||
  		(key == "Reader" || key == "Writer") ||
  		(key == "Qos" && !nodeOpt_.ignoreQos())
  		) {
  		
  		if (nodeOpt_.abbrGUIDs()) {
  			val = abbr(val);
  		}
  		
  		QString text(key.c_str());
  		text += "\n";
  		text += val.c_str();
	
  		Node *node = new Node(text, xpos, ypos, t);
  		
  		// populate tooltips with the children
  		TreeNode *c;
  		std::ostringstream ttip;
  		for (int i = 0; i < t->size(); ++i) {
  			c = t->operator[](i);
  			std::string k = c->column(0).toString().toStdString();   
  			std::string v = c->column(1).toString().toStdString();
  			
  			ttip << k << " : " << v << std::endl;
  		}
  			
  		node->setToolTip(ttip.str().c_str());
  		
  		// this takes ownership of the node so the nodeScene will
  		// cleanup all nodes when it's destroyed
  		nodeScene->addItem(node);
  	}				
  }
  }
  
	// draw the child nodes
	++w;
  for (int i = 0; i < t->size(); ++i) {
  	generate(t->operator[](i), nodeScene, w, ++h);
  }
 
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


