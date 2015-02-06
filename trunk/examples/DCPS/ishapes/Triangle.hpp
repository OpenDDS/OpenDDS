#ifndef _TRIANGLE_HPP
#define _TRIANGLE_HPP

#include <boost/shared_ptr.hpp>
#include <Shape.hpp>
#include <ShapeDynamics.hpp>

class Triangle : public Shape {
public:
  Triangle(const QRect& bounds,
           boost::shared_ptr<ShapeDynamics> dynamics,
           const QPen& pen,
           const QBrush& brush,
           bool targeted = false);

  virtual ~Triangle();

public:
  virtual void update();
  virtual void paint(QPainter& painter);
  virtual void setBounds(const QRect& bounds);

private:
  Triangle(const Triangle&);
  Triangle& operator=(const Triangle&);

private:
  boost::shared_ptr<ShapeDynamics> dynamics_;
  QPolygon triangle_;
};

#endif /* _TRIANGLE_HPP */

