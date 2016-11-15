#ifndef _CIRCLE_HPP
#define _CIRCLE_HPP

#include <Shape.hpp>
#include <ShapeDynamics.hpp>

class Circle : public Shape {
public:

    Circle(const QRect& bounds,
           shared_ptr<ShapeDynamics> dynamics,
           const QPen& pen,
           const QBrush& brush,
           bool targeted = false);

    virtual ~Circle();

public:
    virtual void update();
    virtual void paint(QPainter& painter);
private:
    Circle(const Circle&);
    Circle& operator=(Circle&);

private:
    shared_ptr<ShapeDynamics> dynamics_;
};

#endif /* _CIRCLE_HPP */

