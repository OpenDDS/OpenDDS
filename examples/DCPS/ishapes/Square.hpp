#ifndef _SQUARE_HPP
#define _SQUARE_HPP

#include <QtGui/QtGui>
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
