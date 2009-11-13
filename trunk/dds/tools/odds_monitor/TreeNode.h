/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef TREENODE_H
#define TREENODE_H

#include <QtCore/QList>
#include <QtCore/QVariant>

namespace Monitor {

/**
 * @class TreeNode
 *
 * @brief Encapsulate a node within a tree model.
 *
 * This class provides the interface to interact with elements of a tree
 * model.
 *
 * Note that we hold native Qt containers and types here in order to
 * simplify interaction with the GUI.  While it is quite possible to use
 * STL containers and types here, we would spend a large part of this
 * class translating between the two worldviews.  And that seems a bit
 * silly.
 */
class TreeNode {
  public:
    /**
     * @brief Construct with data and an optional parent link.
     *
     * @param data   list of data contained by this node
     * @param parent link to parent node of this one
     *
     * If no parent link is supplied this node acts as the root of a
     * tree with the parent link held as nil.
     */
    TreeNode( const QList<QVariant>& data, TreeNode* parent = 0);

    /// Virtual destructor.
    virtual ~TreeNode();

    /// Update with new data.
    bool setData( int column, const QVariant& data);

    /// Append another node as a child.
    void append( TreeNode* child);

    /// Insert a number of empty child nodes.
    bool insertChildren( int row, int count, int columns);

    /// Remove a number of child nodes.
    bool removeChildren( int row, int count);

    /**
     * @brief Access a child by index.
     *
     * @param  row the contained child node to select and return
     * @return     pointer to the selected child node
     */
    TreeNode* operator[]( int row);

    /**
     * @brief Access the data in a desired column.
     *
     * @param  column the column of data to access
     * @return        value of the contained data
     */
    QVariant column( int column);

    /// Number of children we have.
    int size() const;

    /// Number of columns that we have.
    int width() const;

    /// Access our index in our parent.
    int row() const;

    /// Access our parent.
    TreeNode* parent();

  private:
    /// Container of children of this element.
    QList<TreeNode*> children_;

    /// Container of data for this node.
    QList<QVariant> data_;

    /// Parent node of this one.  Tree root has nil parent.
    TreeNode* parent_;
};

} // End of namespace Monitor

inline
Monitor::TreeNode::TreeNode(
  const QList<QVariant>& data,
  TreeNode*              parent

) : data_( data),
    parent_( parent)
{
}

inline
Monitor::TreeNode::~TreeNode()
{
  qDeleteAll( this->children_);
}

inline
bool
Monitor::TreeNode::setData( int column, const QVariant& data)
{
  if( column < 0 || column > this->width()) {
    return false;
  }

  this->data_.replace( column, data);
  return true;
}

inline
void
Monitor::TreeNode::append( TreeNode* child)
{
  this->children_.append( child);
}

inline
bool
Monitor::TreeNode::insertChildren( int row, int count, int columns)
{
  if( row < 0 || row > this->size()) {
    return false;
  }

  while( count--) {
    QList< QVariant> data;
    for( int index = 0; index < columns; ++index) {
      data.append( QVariant());
    }
    TreeNode* node = new TreeNode( data, this);
    this->children_.insert( row, node);
  }

  return true;
}

inline
bool
Monitor::TreeNode::removeChildren( int row, int count)
{
  if( row < 0 || (row+count) > this->size()) {
    return false;
  }

  while( count--) {
    delete this->children_.takeAt( row);
  }

  return true;
}

inline
Monitor::TreeNode*
Monitor::TreeNode::operator[]( int row)
{
  return this->children_.value( row);
}

inline
QVariant
Monitor::TreeNode::column( int column)
{
  return this->data_.value( column);
}

inline
int
Monitor::TreeNode::size() const
{
  return this->children_.count();
}

inline
int
Monitor::TreeNode::width() const
{
  return this->data_.count();
}

inline
int
Monitor::TreeNode::row() const
{
  if( this->parent_) {
    return this->parent_->children_.indexOf( const_cast<TreeNode*>(this));

  } else {
    return 0;
  }
}

inline
Monitor::TreeNode*
Monitor::TreeNode::parent()
{
  return this->parent_;
}

#endif /* TREENODE_H */

