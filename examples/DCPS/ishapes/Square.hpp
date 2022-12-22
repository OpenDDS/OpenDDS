#ifndef _SQUARE_HPP
#define _SQUARE_HPP

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtGui/QtGui>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <Shape.hpp>
#include <ShapeDynamics.hpp>

class Square : public Shape {
public:
    Square(const QRect& bounds,
           std::shared_ptr<ShapeDynamics> dynamics,
           const QPen& pen,
           const QBrush& brush,
           bool targeted = false);

    virtual ~Square();

public:
    virtual void update();
    virtual void paint(QPainter& painter);
private:
    Square(const Square& orig);
    Square& operator=(const Square&);

private:
    std::shared_ptr<ShapeDynamics> dynamics_;

};

#endif /* _SQUARE_HPP */
