#ifndef _BOUNCINGSHAPEDYNAMICS_HPP
#define _BOUNCINGSHAPEDYNAMICS_HPP

#include "ShapeTypeTypeSupportC.h"

#include <ShapeDynamics.hpp>
#include <QtCore/QRect>

class BouncingShapeDynamics : public ShapeDynamics {
public:
    BouncingShapeDynamics(int x0, int y0,
                          const QRect& shapeBounds,
                          const QRect& constraints,
                          float speed,
                          float angle,
                          const org::omg::dds::demo::ShapeType& shape,
                          org::omg::dds::demo::ShapeTypeDataWriter_var& shapeWriter);

    virtual ~BouncingShapeDynamics();

public:
    virtual void simulate();

private:
    BouncingShapeDynamics(const BouncingShapeDynamics& orig);
    BouncingShapeDynamics& operator=(const BouncingShapeDynamics&);

private:
    bool flip();

private:
    QRect shapeBounds_;
    float alpha_;
    float angle_;
    float speed_;
    org::omg::dds::demo::ShapeType shape_;
    org::omg::dds::demo::ShapeTypeDataWriter_var  shapeWriter_;
};

#endif /* _BOUNCINGSHAPEDYNAMICS_HPP */

