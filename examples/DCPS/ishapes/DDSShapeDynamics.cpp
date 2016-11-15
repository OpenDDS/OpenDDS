#include "DDSShapeDynamics.hpp"
#include <iostream>

extern char* colorString_[];

using org::omg::dds::demo::ShapeType;
using org::omg::dds::demo::ShapeTypeSeq;
using org::omg::dds::demo::ShapeTypeDataReader_var;

DDSShapeDynamics::DDSShapeDynamics(int x0, int y0,
           ShapeTypeDataReader_var& shapeReader,
           const std::string& color,
           int colorIdx)
  :   ShapeDynamics(x0, y0, QRect(0, 0, 0, 0)),
      x0_(x0),
      y0_(y0),
      shapeReader_(shapeReader),
      attached_(false),
      color_(color),
      colorIdx_(colorIdx),
      updateBounds_(true)
{
  colorList_[BLUE] = QColor(0x33, 0x66, 0x99);
  colorList_[RED] = QColor(0xCC, 0x33, 0x33);
  colorList_[GREEN] = QColor(0x99, 0xCC, 0x66);
  colorList_[ORANGE] = QColor(0xFF, 0x99, 0x33);
  colorList_[YELLOW] = QColor(0xFF, 0xFF, 0x66);
  colorList_[MAGENTA] = QColor(0xCC, 0x99, 0xCC);
  colorList_[CYAN] = QColor(0x99, 0xCC, 0xFF);
  colorList_[GRAY] = QColor(0x99, 0x99, 0x99);
  colorList_[BLACK] = QColor(0x33, 0x33, 0x33);
}

DDSShapeDynamics::~DDSShapeDynamics() {
}

void
DDSShapeDynamics::simulate() {
  ShapeTypeSeq samples;
  DDS::SampleInfoSeq infos;

  shapeReader_->read(samples,
        infos,
        DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE,
        DDS::ANY_VIEW_STATE,
        DDS::ANY_INSTANCE_STATE);

  if (samples.length() > 0) {
    CORBA::Long sampleIndex = -1;
    CORBA::ULong i = 0;

    QPoint tmp;
    plist_.erase(plist_.begin(), plist_.end());
    while (i < samples.length()) {
      if (infos[i].valid_data && strcmp(samples[i].color, color_.c_str()) == 0) {
        tmp.rx() = samples[i].x;
        tmp.ry() = samples[i].y;
        plist_.push_back(tmp);
        sampleIndex = i;
      }
      ++i;
    }

    if (sampleIndex != -1) {
      pos_.rx() = samples[sampleIndex].x;
      pos_.ry() = samples[sampleIndex].y;

      if (attached_ == false) {
        attached_ = true;
        QBrush brush = QBrush(colorList_[colorIdx_], Qt::SolidPattern);
        shape_->setBrush(brush);
      }
      QRect bounds(0,
                   0,
                   samples[sampleIndex].shapesize,
                   samples[sampleIndex].shapesize);
      shape_->setBounds(bounds);
    }
  }
  shapeReader_->return_loan(samples, infos);
}
