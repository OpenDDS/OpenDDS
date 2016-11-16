#include <iostream>
#include <QtGui/QtGui>
#include <ShapesWidget.hpp>

ShapesWidget::ShapesWidget(QWidget *parent)
: QWidget(parent),
  showCurrentFilter_(false),
  logo_(":/images/logo.png")
{
    this->setBackgroundRole(QPalette::Base);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

ShapesWidget::~ShapesWidget() {
}

void
ShapesWidget::addShape(shared_ptr<Shape> shape) {
    shapeList_.push_back(shape);
}

void
ShapesWidget::nextAnimationFrame() {
    this->update();

    ShapeList::iterator index = shapeList_.begin();
    while (index != shapeList_.end()) {
        (*index)->update();
        ++index;
    }
}

void
ShapesWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(0, height() - logo_.height(), logo_);
    if (showCurrentFilter_) {
      QBrush brush(QColor(0x99,0x99,0x99,0x99), Qt::SolidPattern);
      painter.setBrush(brush);
      painter.drawRect(currentFilter_);
    }
    ShapeList::iterator index = shapeList_.begin();
    while (index != shapeList_.end()) {
        (*index)->paint(painter);
        ++index;
    }
}

void
ShapesWidget::addFilter(const QRect& filter) {
  filterList_.push_back(filter);
}

void ShapesWidget::displayFilter(const QRect& currentFilter) {
  currentFilter_ = currentFilter;
  showCurrentFilter_ = true;
  this->update();
}
