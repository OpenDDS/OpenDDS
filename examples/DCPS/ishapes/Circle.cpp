#include <QtGui/qpainter.h>
#include <vector>
#include "Circle.hpp"


Circle::Circle(const QRect& bounds,
               shared_ptr<ShapeDynamics> dynamics,
               const QPen& pen,
               const QBrush& brush,
               bool targeted)
    :   Shape(bounds, pen, brush, targeted),
        dynamics_(dynamics)
{ }

void
Circle::update() {
    dynamics_->simulate();
}

void
Circle::paint(QPainter& painter) {
    painter.setBrush(brush_);
    painter.setPen(pen_);
    std::vector<QPoint> plist = dynamics_->getPositionList();
    std::vector<QPoint>::iterator idx = plist.begin();

    QBrush black( QColor(0x33, 0x33, 0x33), Qt::SolidPattern);
    QBrush white( QColor(0xFF, 0xFF, 0xFF), Qt::SolidPattern);
    QBrush brush;
    while (idx != plist.end()) {
      painter.drawEllipse(idx->x() - bounds_.width()/2,
        idx->y() - bounds_.height()/2,
        bounds_.width(),
        bounds_.height());
      if (targeted_)
        brush = black;
      else
        brush = white;

      int X0 = idx->x();
      int Y0 = idx->y();
      int W = bounds_.width()/3;
      int H = bounds_.height()/3;
      painter.setBrush(brush);
      painter.drawEllipse(X0 - W/2,
                          Y0 - H/2,
                          W,
                          H);
      painter.setBrush(brush_);

      ++idx;
    }
}

Circle::~Circle() {
}
