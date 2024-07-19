#include <iostream>

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtGui/QtGui>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <ShapesWidget.hpp>

#include "dds/DCPS/Service_Participant.h"

#include <dds/OpenDDSConfigWrapper.h>

namespace {
  const char* logoFile()
  {
    return
#if OPENDDS_CONFIG_SECURITY
      TheServiceParticipant->get_security() ? ":/images/logo_secure_beta.png" :
#endif
      ":/images/logo.png";
  }
}

ShapesWidget::ShapesWidget(QWidget *parent)
: QWidget(parent),
  showCurrentFilter_(false),
  logo_(logoFile())
{
    this->setBackgroundRole(QPalette::Base);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

ShapesWidget::~ShapesWidget() {
}

void
ShapesWidget::addShape(std::shared_ptr<Shape> shape) {
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
