#include "BouncingShapeDynamics.hpp"
#include <math.h>
#include <stdlib.h>

#include <iostream>
static const float PI = 3.1415926535F;

#ifdef WIN32
#define roundf(a) ((a)>0?floor((a)+0.5):ceil((a)-0.5))
#endif

using org::omg::dds::demo::ShapeType;
using org::omg::dds::demo::ShapeTypeDataWriter_var;

BouncingShapeDynamics::BouncingShapeDynamics(int x0, int y0,
                                             const QRect& shapeBounds,
                                             const QRect& constraint,
                                             float angle,
                                             float speed,
                                             const ShapeType& shape,
                                             ShapeTypeDataWriter_var& shapeWriter)
  :   ShapeDynamics(x0, y0, constraint),
      shapeBounds_(shapeBounds),
      alpha_(angle),
      angle_(angle),
      speed_(speed),
      shape_(shape),
      shapeWriter_(shapeWriter)
{ }


BouncingShapeDynamics::~BouncingShapeDynamics()
{ }

bool
BouncingShapeDynamics::flip() {
  bool doflip = false;
  if (rand() <= RAND_MAX/2)
    doflip = true;

  return doflip;
}

void
BouncingShapeDynamics::simulate()
{
  pos_.rx() = roundf(pos_.rx() + speed_*cosf(angle_));
  pos_.ry() = roundf(pos_.ry() + speed_*sinf(angle_));

  if (pos_.x() <= shapeBounds_.width()/2) {
    angle_ = this->flip() ? -alpha_ : alpha_;
    pos_.rx() = shapeBounds_.width()/2;
  }
  else if (pos_.x() >= (constraint_.width() - (shapeBounds_.width()/2))) {
    angle_ = this->flip() ? (PI + alpha_) : (PI - alpha_);
    pos_.rx() = constraint_.width() - shapeBounds_.width()/2;
  }
  else if (pos_.y() <= shapeBounds_.height()/2) {
    angle_ = this->flip() ? alpha_ : PI - alpha_;
    pos_.ry() = shapeBounds_.height()/2;
  }
  else if (pos_.y() >= (constraint_.height() - shapeBounds_.height()/2)) {
    angle_ = this->flip() ? (PI+alpha_) : -alpha_;
    pos_.ry() = constraint_.height() - shapeBounds_.height()/2;
  }

  shape_.x = pos_.x();
  shape_.y = pos_.y();
  shapeWriter_->write(shape_, ::DDS::HANDLE_NIL);

  plist_.erase(plist_.begin(), plist_.end());
  plist_.push_back(pos_);
}
