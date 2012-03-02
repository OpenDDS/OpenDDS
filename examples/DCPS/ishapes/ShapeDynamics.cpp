#include "ShapeDynamics.hpp"

ShapeDynamics::ShapeDynamics(int x0, int y0)
    : pos_(x0, y0)
{
  plist_.push_back(pos_);
}

ShapeDynamics::ShapeDynamics(int x0, int y0, const QRect& constraint)
    :   pos_(x0, y0),
        constraint_(constraint)
{
  plist_.push_back(pos_);
}

ShapeDynamics::~ShapeDynamics() {
}

QPoint
ShapeDynamics::getPosition()
{
  return pos_;
}

std::vector<QPoint>
ShapeDynamics::getPositionList()
{
  return plist_;
}

void
ShapeDynamics::setConstraint(const QRect& rect) {
    constraint_ = rect;
}
