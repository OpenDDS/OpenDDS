#include "config.hpp"
#include "ShapesDialog.hpp"
#include <QtGui/QtGui>
#include <iostream>
#include <sstream>
#include <Circle.hpp>
#include <Square.hpp>
#include <Triangle.hpp>
#include <BouncingShapeDynamics.hpp>
#include <DDSShapeDynamics.hpp>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include "ace/config-all.h"
#include "ShapeTypeTypeSupportImpl.h"

#ifdef ACE_HAS_CPP11
# include <string>
# define TO_STRING std::to_string
#else
# include <boost/lexical_cast.hpp>
# define TO_STRING boost::lexical_cast<std::string>
#endif

using org::omg::dds::demo::ShapeType;
using org::omg::dds::demo::ShapeTypeTypeSupport_var;
using org::omg::dds::demo::ShapeTypeTypeSupportImpl;
using org::omg::dds::demo::ShapeTypeDataWriter;
using org::omg::dds::demo::ShapeTypeDataWriter_var;
using org::omg::dds::demo::ShapeTypeDataReader;
using org::omg::dds::demo::ShapeTypeDataReader_var;

static const float PI = 3.1415926535F;

static QColor  color_[CN];

const char* const colorString_[] = {
  "BLUE",
  "RED",
  "GREEN",
  "ORANGE",
  "YELLOW",
  "MAGENTA",
  "CYAN",
  "GRAY",
  "BLACK"
};

static const std::string circleTopicName("Circle");
static const std::string squareTopicName("Square");
static const std::string triangleTopicName("Triangle");


ShapesDialog::ShapesDialog(DDS::DomainParticipant_var participant,
                           const std::string& partition)
  :   timer(this),
      participant_(participant),
      filterExpression_("(x BETWEEN %0 AND %1) AND (y BETWEEN %2 AND %3)")
{
  DDS::TopicQos topic_qos;
  participant->get_default_topic_qos(topic_qos);
  topic_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  topic_qos.history.depth = 100;

  // Register TypeSupport (ShapeType)
  ShapeTypeTypeSupport_var ts =
    new ShapeTypeTypeSupportImpl;

  CORBA::String_var type_name = "ShapeType";
  if (ts->register_type(participant, type_name) != DDS::RETCODE_OK) {
      std::cerr << "Could not register type " << std::endl;
  }

  circleTopic_ =
    participant->create_topic(circleTopicName.c_str(),
                              type_name,
                              topic_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!circleTopic_) {
      std::cerr << "Could not create topic " << circleTopicName << std::endl;
  }

  squareTopic_ =
    participant->create_topic(squareTopicName.c_str(),
                              type_name,
                              topic_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!squareTopic_) {
      std::cerr << "Could not create topic " << squareTopicName << std::endl;
  }

  triangleTopic_ =
    participant->create_topic(triangleTopicName.c_str(),
                              type_name,
                              topic_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!triangleTopic_) {
      std::cerr << "Could not create topic " << triangleTopicName << std::endl;
  }

  DDS::PublisherQos pub_qos;
  participant->get_default_publisher_qos(pub_qos);
  if (!partition.empty()) {
    pub_qos.partition.name.length(1);
    pub_qos.partition.name[0] = partition.c_str();
  }
  // Create Publisher
  publisher_ =
    participant->create_publisher(pub_qos,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!publisher_) {
      std::cerr << "Could not create publisher " << std::endl;
  }
  writerQos_.setPublisher(publisher_);

  DDS::SubscriberQos sub_qos;
  participant->get_default_subscriber_qos(sub_qos);
  if (!partition.empty()) {
    sub_qos.partition.name.length(1);
    sub_qos.partition.name[0] = partition.c_str();
  }
  // Create Subscriber
  subscriber_ =
    participant->create_subscriber(sub_qos,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber_) {
      std::cerr << "Could not create subscriber " << std::endl;
  }

  readerQos_.setSubscriber(subscriber_);

  mainWidget.setupUi(this);
  shapesWidget = new ShapesWidget(mainWidget.renderFrame);
  shapesWidget->resize(mainWidget.renderFrame->size());
  filterDialog_ = new FilterDialog(shapesWidget);
  connect(&timer, SIGNAL(timeout()),
    shapesWidget, SLOT(nextAnimationFrame()));

  color_[BLUE] = QColor(0x33, 0x66, 0x99);
  color_[RED] = QColor(0xCC, 0x33, 0x33);
  color_[GREEN] = QColor(0x99, 0xCC, 0x66);
  color_[ORANGE] = QColor(0xFF, 0x99, 0x33);
  color_[YELLOW] = QColor(0xFF, 0xFF, 0x66);
  color_[MAGENTA] = QColor(0xCC, 0x99, 0xCC);
  color_[CYAN] = QColor(0x99, 0xCC, 0xFF);
  color_[GRAY] = QColor(0x99, 0x99, 0x99);
  color_[BLACK] = QColor(0x33, 0x33, 0x33);

  const int index = mainWidget.colorList->findText("Green");
  mainWidget.colorList->setCurrentIndex(index);

  timer.start(40);

  if (partition.length()) {
    QString title = this->windowTitle();
    title += (" PARTITION: " + partition).c_str();
    this->setWindowTitle(title);
  }
}

ShapesDialog::~ShapesDialog() {
  delete filterDialog_;
  delete shapesWidget;
}

void
ShapesDialog::onPublishButtonClicked() {
  int d = mainWidget.sizeSlider->value();
  float speed = ((float)mainWidget.speedSlider->value()) / 9;
  QRect rect(0, 0, d, d);
  // TODO: This should be retrieved from the canvas...


  QRect constr(0, 0, IS_WIDTH, IS_HEIGHT);
  // QRect constr = this->geometry();
  int x = constr.width() * ((float)rand() / RAND_MAX);
  int y = constr.height() * ((float)rand() / RAND_MAX);
  int cIdx = mainWidget.colorList->currentIndex();
  int sIdx = mainWidget.wShapeList->currentIndex();

  QBrush brush(color_[cIdx], Qt::SolidPattern);
  //  QPen pen(color_[(cIdx+1)%(CN-1)], 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  // color_[cIdx]
  // QColor(33, 33, 33)
  QPen pen(QColor(0xff, 0xff, 0xff), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

  switch (sIdx) {
  case CIRCLE: {
    ShapeType shape;
    shape.color = CORBA::string_dup(colorString_[cIdx]);
    shape.shapesize = rect.width();
    shape.x = x;
    shape.y = y;

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher_->create_datawriter(circleTopic_,
                                   writerQos_.get_qos(),
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
        std::cerr << "Could not create data writer " << std::endl;
    }
    ShapeTypeDataWriter_var dw =
      ShapeTypeDataWriter::_narrow(writer);

    shared_ptr<BouncingShapeDynamics>
      dynamics(new BouncingShapeDynamics(x, y, rect, constr, PI/6, speed,
           shape, dw));
    shared_ptr<Shape>
      circle(new Circle(rect, dynamics, pen, brush));
    shapesWidget->addShape(circle);

    break;
  }
  case SQUARE: {
    ShapeType shape;
    shape.color = CORBA::string_dup(colorString_[cIdx]);
    shape.shapesize = rect.width();
    shape.x = x;
    shape.y = y;

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher_->create_datawriter(squareTopic_,
                                   writerQos_.get_qos(),
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
        std::cerr << "Could not create data writer " << std::endl;
    }
    ShapeTypeDataWriter_var dw =
      ShapeTypeDataWriter::_narrow(writer);

    shared_ptr<BouncingShapeDynamics>
      dynamics(new BouncingShapeDynamics(x, y, rect, constr, PI/6, speed,
           shape, dw));
    shared_ptr<Shape>
      square(new Square(rect, dynamics, pen, brush));
    shapesWidget->addShape(square);
    // std::cout << "CREATE SQUARE" << std::endl;
    break;
  }
  case TRIANGLE: {
    ShapeType shape;
    shape.color = CORBA::string_dup(colorString_[cIdx]);
    shape.shapesize = rect.width();
    shape.x = x;
    shape.y = y;

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher_->create_datawriter(triangleTopic_,
                                   writerQos_.get_qos(),
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
        std::cerr << "Could not create data writer " << std::endl;
    }
    ShapeTypeDataWriter_var dw =
      ShapeTypeDataWriter::_narrow(writer);

    shared_ptr<BouncingShapeDynamics>
      dynamics(new BouncingShapeDynamics(x, y, rect, constr, PI/6, speed,
           shape, dw));
    shared_ptr<Shape>
      triangle(new Triangle(rect, dynamics, pen, brush));
    shapesWidget->addShape(triangle);
    // std::cout << "CREATE TRIANGLE" << std::endl;
    break;
  }
  default:
    break;
  };

}

void
ShapesDialog::onSubscribeButtonClicked() {

  const int d = mainWidget.sizeSlider->value();
  QRect rect(0, 0, d, d);
  QRect constr(0, 0, IS_WIDTH, IS_HEIGHT);
  // QRect constr = this->geometry();
  int x = static_cast<int>(constr.width() * ((float)rand() / RAND_MAX)*0.9F);
  int y = static_cast<int>(constr.height() * ((float)rand() / RAND_MAX)*0.9F);
  int sIdx = mainWidget.rShapeList->currentIndex();

  QColor gray = QColor(0x99, 0x99, 0x99);
  //  QBrush brush(color_[BLACK], Qt::FDiagPattern);
  QBrush brush(gray, Qt::SolidPattern);

  QPen pen(QColor(0xff,0xff,0xff), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

  filterParams_ = DDS::StringSeq();
  if (filterDialog_->isEnabled()) {
    QRect rect =  filterDialog_->getFilterBounds();
    std::string x0 = TO_STRING(rect.x());
    std::string x1 = TO_STRING(rect.x() + rect.width());

    std::string y0 = TO_STRING(rect.y());
    std::string y1 = TO_STRING(rect.y() + rect.height());
    filterParams_.length(4);
    filterParams_[0] = x0.c_str();
    filterParams_[1] = x1.c_str();
    filterParams_[2] = y0.c_str();
    filterParams_[3] = y1.c_str();
  }

  ShapeTypeDataReader_var dr;

  DDS::TopicDescription_var topicDesc;
  std::stringstream tname;

  const std::string filter = !filterDialog_->isEnabled() ? "" :
                             (!filterDialog_->filterOutside() ?
                              "(x < %0) OR (x > %1) OR (y < %2) OR (y > %3)" :
                              filterExpression_) ;
  if (filterDialog_->isEnabled()) {
    std::cout << "creating cft with filter=" << filter << std::endl;
    std::cout << "%0=" << filterParams_[0] << ", %1=" << filterParams_[1] << ", %2=" << filterParams_[2] << ", %3=" << filterParams_[3] << std::endl;
  }
  switch (sIdx) {
  case CIRCLE: {
    if (filterDialog_->isEnabled()) {
      static unsigned int count = 0;
      tname << "CFCircle" << ++count;

      topicDesc = participant_->create_contentfilteredtopic(
        tname.str().c_str(), circleTopic_, filter.c_str(), filterParams_);
    }
    else {
      topicDesc = DDS::TopicDescription::_duplicate(circleTopic_.in());
    }
    break;
  }

  case SQUARE: {
    if (filterDialog_->isEnabled()) {
      static unsigned int count = 0;
      tname << "CFSquare" << ++count;

      topicDesc = participant_->create_contentfilteredtopic(
        tname.str().c_str(), squareTopic_, filter.c_str(), filterParams_);
    }
    else {
      topicDesc = DDS::TopicDescription::_duplicate(squareTopic_.in());
    }
    break;
  }
  case TRIANGLE: {
    if (filterDialog_->isEnabled()) {
      static unsigned int count = 0;
      tname << "CFTriangle" << ++count;

      topicDesc = participant_->create_contentfilteredtopic(
        tname.str().c_str(), triangleTopic_, filter.c_str(), filterParams_);
    }
    else {
      topicDesc = DDS::TopicDescription::_duplicate(triangleTopic_.in());
    }
    break;
  }

  default:
    break;
  }

  DDS::DataReader_var reader =
    subscriber_->create_datareader(topicDesc,
                           readerQos_.get_qos(),
                           0,
                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!reader) {
    std::cerr << "Could not create data reader " << std::endl;
  }

  dr =
    ShapeTypeDataReader::_narrow(reader);

  if (!dr) {
    std::cerr << "Data reader is not a ShapeTypeDataReader " << std::endl;
  }

  switch (sIdx) {
  case CIRCLE: {
    for (int i = 0; i < CN; ++i) {
      std::string colorStr(colorString_[i]);
      shared_ptr<DDSShapeDynamics>
        dynamics(new DDSShapeDynamics(x, y, dr, colorStr, i));
      shared_ptr<Shape>
        circle(new Circle(rect, dynamics, pen, brush, true));
      dynamics->setShape(circle);
      shapesWidget->addShape(circle);
    }
    break;
  }

  case SQUARE: {
    for (int i = 0; i < CN; ++i) {
      std::string colorStr(colorString_[i]);
      shared_ptr<DDSShapeDynamics>
        dynamics(new DDSShapeDynamics(x, y, dr, colorStr, i));
      shared_ptr<Shape>
        square(new Square(rect, dynamics, pen, brush, true));
      dynamics->setShape(square);
      shapesWidget->addShape(square);
    }
    break;
  }
  case TRIANGLE: {
    for (int i = 0; i < CN; ++i) {
      std::string colorStr(colorString_[i]);
      shared_ptr<DDSShapeDynamics>
        dynamics(new DDSShapeDynamics(x, y, dr, colorStr, i));
      shared_ptr<Shape>
        triangle(new Triangle(rect, dynamics, pen, brush, true));
      dynamics->setShape(triangle);
      shapesWidget->addShape(triangle);
    }
    break;
  }

  default:
    break;
  }

}

void
ShapesDialog::onReaderQosButtonClicked()
{
  readerQos_.setVisible(true);
}
void
ShapesDialog::onWriterQosButtonClicked()
{
  writerQos_.setVisible(true);
}

void
ShapesDialog::onFilterButtonClicked()
{
  filterDialog_->setVisible(true);
}
