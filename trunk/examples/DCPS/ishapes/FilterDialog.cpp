#include <FilterDialog.hpp>

FilterDialog::FilterDialog(ShapesWidget* widget)
: widget_(widget),
  enabled_(false)
{
  filterDialog_.setupUi(this);
  this->setVisible(false);
  int X0 = filterDialog_.x0Slider->value();
  int Y0 = filterDialog_.y0Slider->value();
  int X1 = filterDialog_.x1Slider->value();
  int Y1 = filterDialog_.y1Slider->value();
  int W = X1 - X0;
  int H =  Y1 - Y0;
  filter_ = QRect(X0,Y0, W, H);
}

FilterDialog::~FilterDialog() { }

void
FilterDialog::accept() {
  this->setVisible(false);
}

void
FilterDialog::reject() {
  this->setVisible(false);
}

void
FilterDialog::updateX0(int x) {
  filter_.setX(x);
  if (enabled_)
    widget_->displayFilter(filter_);
}

void
FilterDialog::updateY0(int y) {
  filter_.setY(y);
  if (enabled_)
    widget_->displayFilter(filter_);
}

void
FilterDialog::updateX1(int x) {
  filter_.setWidth(abs(x-filter_.x()));
  if (enabled_)
    widget_->displayFilter(filter_);
}

void
FilterDialog::updateY1(int y) {
  filter_.setHeight(abs(y-filter_.y()));
  if (enabled_)
    widget_->displayFilter(filter_);
}

void
FilterDialog::updateFilterStatus(bool on) {
  if (on)
    widget_->displayFilter(filter_);
  enabled_ = on;
}

bool
FilterDialog::isEnabled() {
  return enabled_;
}

QRect
FilterDialog::getFilterBounds() {
  return filter_;
}

bool
FilterDialog::filterOutside() {
  return filterDialog_.filterCombo->currentIndex() == 0;
}
