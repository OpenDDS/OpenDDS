#ifndef _DDSSHAPEDYNAMICS_HPP
#define _DDSSHAPEDYNAMICS_HPP

#include "ShapeTypeTypeSupportC.h"
#include <ShapeDynamics.hpp>
#include <QtCore/QRect>
#include <QtGui/QtGui>
#include <Shape.hpp>

#define CN 9

class DDSShapeDynamics : public ShapeDynamics {
public:
  enum {
    BLUE    = 0,
    RED     = 1,
    GREEN   = 2,
    ORANGE  = 3,
    YELLOW  = 4,
    MAGENTA = 5,
    CYAN    = 6,
    GRAY    = 7,
    BLACK   = 8
  };
public:
  DDSShapeDynamics(
       int x0, int y0,
       org::omg::dds::demo::ShapeTypeDataReader_var& shapeReader,
       const std::string& color,
       int colorIdx);

  virtual ~DDSShapeDynamics();

public:
  void setShape(shared_ptr<Shape> shape) {
    shape_ = shape;
  }

  virtual void simulate();
private:
  DDSShapeDynamics(const DDSShapeDynamics& orig);

  shared_ptr<Shape> shape_;
  int x0_;
  int y0_;
  org::omg::dds::demo::ShapeTypeDataReader_var shapeReader_;
  bool attached_;
  std::string color_;
  int colorIdx_;
  QColor  colorList_[CN];
  bool updateBounds_;
};

#endif /* _DDSSHAPEDYNAMICS_HPP */

