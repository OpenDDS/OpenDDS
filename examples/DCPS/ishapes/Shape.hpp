#ifndef _SHAPE_HPP
#define _SHAPE_HPP

#include <QtCore/QRect>
#include <QtGui/QPen>
#include <QtGui/QBrush>

#include "ace/config-all.h"

#if defined (ACE_HAS_CPP11)
# include <memory>
using std::shared_ptr;
#else
# include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#endif /* ACE_HAS_CPP11 */


class Shape {
public:
    Shape(const QRect& bounds,
          const QPen& pen,
          const QBrush& brush,
          bool targeted = false);

    virtual ~Shape();
public:
    virtual void update() = 0;
    virtual void paint(QPainter& painter) = 0;

public:
    virtual void setPen(const QPen& pen);
    virtual void setBrush(const QBrush& brush);
    virtual void setBounds(const QRect& bounds);
    virtual void set_targeted(bool b);

private:
    Shape(const Shape&);
    Shape& operator=(const Shape&);

protected:
    QRect bounds_;
    QPen pen_;
    QBrush brush_;
    bool targeted_;
};

#endif /* _SHAPE_HPP */
