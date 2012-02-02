#include "Shape.hpp"
#include <iostream>

Shape::Shape(const QRect& bounds, const QPen& pen, const QBrush& brush, bool targeted)
    :   bounds_(bounds),
        pen_(pen),
        brush_(brush),
        targeted_(targeted)
{}

Shape::~Shape() {
}

void
Shape::setPen(const QPen& pen) {
    pen_ = pen;
}

void
Shape::setBrush(const QBrush& brush) {
    brush_ = brush;
}

void
Shape::setBounds(const QRect& bounds) {
    bounds_ = bounds;
}


void
Shape::set_targeted(bool b) { targeted_ = b;}
