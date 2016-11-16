#include <QtGui/qpainter.h>

#include "Triangle.hpp"

Triangle::Triangle(const QRect& bounds,
                   shared_ptr<ShapeDynamics> dynamics,
                   const QPen& pen,
                   const QBrush& brush,
                   bool targeted)
    :   Shape(bounds, pen, brush, targeted),
        dynamics_(dynamics)
{
    QPoint p1(bounds_.width()/2, 0);
    QPoint p2(0, bounds_.width());
    QPoint p3(bounds_.width(), bounds_.width());

    triangle_ << p1 << p2 << p3;
}

Triangle::~Triangle() {
}

void
Triangle::setBounds(const QRect& bounds)
{
  bounds_ = bounds;
  QPoint p1(bounds_.width()/2, 0);
  QPoint p2(0, bounds_.width());
  QPoint p3(bounds_.width(), bounds_.width());

  QPolygon triangle;
  triangle << p1 << p2 << p3;
  triangle_ = triangle;
}
void
Triangle::update() {
    dynamics_->simulate();
}

void
Triangle::paint(QPainter& painter) {
    painter.setBrush(brush_);
    painter.setPen(pen_);

    std::vector<QPoint> plist = dynamics_->getPositionList();
    std::vector<QPoint>::iterator idx = plist.begin();

    QBrush black( QColor(0x33, 0x33, 0x33), Qt::SolidPattern);
    QBrush white( QColor(0xFF, 0xFF, 0xFF), Qt::SolidPattern);
    QBrush brush;

    while (idx != plist.end()) {
      painter.translate(idx->x() - bounds_.width()/2,
                        idx->y() - bounds_.height()/2);
      painter.drawPolygon(triangle_);
      painter.translate(-(idx->x() - bounds_.width()/2),
                        -(idx->y() - bounds_.height()/2));

      if (targeted_)
        brush = black;
      else
        brush = white;

      painter.setBrush(brush);
      int X0 = idx->x();
      int Y0 = idx->y() + (bounds_.height()/6);
      int W = bounds_.width()/3;
      int H = bounds_.height()/3;
      painter.setBrush(brush);
      painter.drawEllipse(X0 - W/2,
        Y0 - H/2,
        W,
        H);
      painter.setBrush(brush_);
      painter.setBrush(brush_);

      ++idx;
    }
}

