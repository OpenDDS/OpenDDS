/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TREENODE_H
#define TREENODE_H

#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QtAlgorithms>
#include <QtGui/QColor>
#include <QtGui/QCheckBox>

namespace Monitor {

/**
 * @class TreeNode
 *
 * @brief Encapsulate a node within a tree model.
 *
 * This class provides the interface for data to interact with elements
 * of a tree model.
 *
 * Note that we hold native Qt containers and types here in order to
 * simplify interaction with the GUI.  While it is quite possible to use
 * STL containers and types here, we would spend a large part of this
 * class translating between the two worldviews.  And that seems a bit
 * silly.
 *
 * Since it is desirable to display data in multiple locations within
 * the tree, this class implements the concept of 'value reference' nodes
 * which obtain their data from a single source.  This allows the GUI to
 * represent the same data at multiple locations in the tree in a way
 * that ensures consistent data views.  A tree node with a non-NULL
 * valueSource_ member is called a "value reference" node.  The tree node
 * located as the valueSource_ will be queried for all data information
 * when the GUI requests information for this node.  The location is
 * local to this node, keeping it separate from the referenced node.
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

    virtual ~TreeNode();

    /// Update with new data.
    bool setData( int column, const QVariant& data);

    /// Update with new background color.
    bool setColor( int column, const QVariant& data);

    /// Append another node as a child.
    void append( TreeNode* child);

    /// Append another node as a reference to the value of this data.
    void addValueRef( TreeNode* ref);

    /// Remove a value reference node for this data.
    bool removeValueRef( TreeNode* ref);

    /// Reassign all references to this node to a new node.
    bool reassignValueRefs( TreeNode* node);

    /// Value reference data source is no longer valid.
    void staleValueRef();

    /// Insert a number of empty child nodes.
    bool insertChildren( int row, int count, int columns);

    /// Remove a number of child nodes.
    bool removeChildren( int row, int count);

    /// Sort our children recursively.
    void sort( int column, Qt::SortOrder order);

    /// Function object defining how to sort children.
    class CompareByColumn {
      public:
        CompareByColumn( int column, Qt::SortOrder order);
        bool operator()(
               const TreeNode* const& lhs,
               const TreeNode* const& rhs
             ) const;
      private:
        int           column_;
        Qt::SortOrder order_;
    };

    /**
     * @brief Access a child by index.
     *
     * @param  row the contained child node to select and return
     * @return     pointer to the selected child node
     */
    TreeNode* operator[]( int row) const;

    /**
     * @brief Access the data in a desired column.
     *
     * @param  column the column of data to access
     * @return        value of the contained data
     */
    QVariant column( int column) const;

    /// Get the background color of a column.
    QVariant color( int column) const;

    /**
     * @brief find the row of a value in a colum.
     *
     * @param column the column to be searched
     * @param value  the value to compare
     * @return       the row containing the value
     */
    int indexOf( int column, QVariant value);

    /// Number of children we have.
    int size() const;

    /// Number of columns that we have.
    int width() const;

    /// Access our index in our parent.
    int row() const;

    /// Access our parent.
    TreeNode*  parent() const;
    TreeNode*& parent();

    QList<TreeNode*> children() const;

    /// Access the source node for our data, if different from us.
    TreeNode*  valueSource() const;
    TreeNode*& valueSource();

    /// Display accessors
    void setDisplay(const bool flag = false);
    void resetDisplays();
    bool display() const;

  private:

    /// Container of children of this element.
    QList<TreeNode*> children_;

    /// Container of different tree nodes that view this same data.
    QList<TreeNode*> valueRefs_;

    /// Container of data for this node.
    QList<QVariant> data_;

    /// Container of background colors for this node.
    QList<QVariant> colors_;

    /// Parent node of this one.  Tree root has nil parent.
    TreeNode* parent_;

    /// Data source if this is a reference value.
    TreeNode* valueSource_;

    /// display for graphviz / qt
    bool display_;

};

} // End of namespace Monitor

inline
Monitor::TreeNode::CompareByColumn::CompareByColumn(
  int column,
  Qt::SortOrder order
) : column_( column), order_( order)
{
}

inline
bool
Monitor::TreeNode::CompareByColumn::operator()(
  const TreeNode* const& lhs,
  const TreeNode* const& rhs
) const
{
  switch( this->order_) {
    case Qt::AscendingOrder:
      return rhs->column( this->column_).toString()
           < lhs->column( this->column_).toString();

    case Qt::DescendingOrder:
      return lhs->column( this->column_).toString()
           < rhs->column( this->column_).toString();
  }
  // Unrecognized order will result in equality for all.
  return false;
}

inline
Monitor::TreeNode::TreeNode(
  const QList<QVariant>& data,
  TreeNode*              parent

) : data_( data),
    parent_( parent),
    valueSource_( 0),
    display_ (false)
{
}

inline
Monitor::TreeNode::~TreeNode()
{
  // Cascade delete children.
  qDeleteAll( this->children_);

  // Remove references from this node.
  if( this->valueSource_) {
    this->valueSource_->removeValueRef( this);
  }

  // Remove references to this node.
  while( !this->valueRefs_.isEmpty()) {
    this->valueRefs_.takeFirst()->staleValueRef();
  }
}

inline
bool
Monitor::TreeNode::setData( int column, const QVariant& data)
{
  if( column < 0 || column > this->width()) {
    return false;
  }

  if( this->valueSource_) {
    // Forward new data to the referent.
    this->valueSource_->setData( column, data);

  } else {
    this->data_.replace( column, data);
  }

  return true;
}

inline
bool
Monitor::TreeNode::setColor( int column, const QVariant& data)
{
  if( column < 0 || column > this->width()) {
    return false;
  }

  if( column < this->colors_.count()) {
    this->colors_.replace( column, data);

  } else {
    for( int index = this->colors_.count(); index < column; ++index) {
      this->colors_.push_back( QVariant(QColor()));
    }
    this->colors_.insert( column, data);
  }

  return true;
}

inline
void
Monitor::TreeNode::append( TreeNode* child)
{
  this->children_.append( child);
}

inline
void
Monitor::TreeNode::addValueRef( TreeNode* ref)
{
  this->valueRefs_.append( ref);
  ref->valueSource() = this;
}

inline
bool
Monitor::TreeNode::removeValueRef( TreeNode* ref)
{
  ref->staleValueRef();
  return this->valueRefs_.removeAll( ref);
}

inline
bool
Monitor::TreeNode::reassignValueRefs( TreeNode* node)
{
  bool value = !this->valueRefs_.isEmpty();
  while( !this->valueRefs_.isEmpty()) {
    node->addValueRef( this->valueRefs_.takeFirst());
  }
  return value;
}

inline
void
Monitor::TreeNode::staleValueRef()
{
  for( int index = 0; index < this->width(); ++index) {
    this->data_.insert( index, QString( QObject::tr( "<defunct>")));
    this->setColor( index, QColor("yellow"));
  }
  this->valueSource_ = 0;
}

inline
bool
Monitor::TreeNode::insertChildren( int row, int count, int columns)
{
  if( row < 0 || row > this->size()) {
    return false;
  }

  QList< QVariant> data;
  QList< QVariant> colors;
  for( int index = 0; index < columns; ++index) {
    data.append( QVariant());
    colors.append( QColor());
  }

  while( count--) {
    TreeNode* node = new TreeNode( data, this);
    for( int index = 0; index < columns; ++index) {
      node->setColor( index, colors[ index]);
    }
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
void
Monitor::TreeNode::sort( int column, Qt::SortOrder order)
{
  CompareByColumn compare( column, order);
  qStableSort( this->children_.begin(), this->children_.end(), compare);
  for( int index = 0; index < this->size(); ++index) {
    (*this)[ index]->sort( column, order);
  }
}

inline
Monitor::TreeNode*
Monitor::TreeNode::operator[]( int row) const
{
  return this->children_.value( row);
}

inline
int
Monitor::TreeNode::indexOf( int column, QVariant value)
{
  for( int index = 0; index < this->size(); ++index) {
    if( value == (*this)[ index]->column( column)) {
      return index;
    }
  }
  return -1;
}

inline
QVariant
Monitor::TreeNode::column( int column) const
{
  if( this->valueSource_) {
    // This is a reference node, use the referenced value.
    return this->valueSource_->column( column);

  } else {
    // This node contains the value;
    return this->data_.value( column);
  }
}

inline
QVariant
Monitor::TreeNode::color( int column) const
{
  // Colors are local.
  return this->colors_.value( column);
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
  if( this->valueSource_) {
    return this->valueSource_->width();

  } else {
    return this->data_.count();
  }
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
Monitor::TreeNode::parent() const
{
  return this->parent_;
}

inline
Monitor::TreeNode*&
Monitor::TreeNode::parent()
{
  return this->parent_;
}

inline
QList<Monitor::TreeNode*>
Monitor::TreeNode::children() const
{
  return this->children_;
}

inline
Monitor::TreeNode*
Monitor::TreeNode::valueSource() const
{
  return this->valueSource_;
}

inline
Monitor::TreeNode*&
Monitor::TreeNode::valueSource()
{
  return this->valueSource_;
}

inline
void Monitor::TreeNode::setDisplay(const bool flag)
{
  this->display_ = flag;
}

inline
void Monitor::TreeNode::resetDisplays()
{
  this->display_ = false;

  for (int i = 0; i < this->children_.size(); ++i) {
    (this->children_.at(i))->resetDisplays();
  }
}

inline
bool Monitor::TreeNode::display() const
{
  return this->display_;
}

#endif /* TREENODE_H */

