
#ifndef _ISHAPESFORM_HPP
#define _ISHAPESFORM_HPP

#include "ShapeTypeTypeSupportC.h"

#include <QtGui/QtGui>

#include <ui_iShapesForm.h>
#include <WriterQosDialog.hpp>
#include <ReaderQosDialog.hpp>
#include <FilterDialog.hpp>

#include <ShapesWidget.hpp>
#include <Circle.hpp>

#define CN 9


//#include <topic-traits.hpp>
class ShapesDialog : public QDialog {
  Q_OBJECT
  public:
  enum { CIRCLE = 0, SQUARE = 1, TRIANGLE = 2 };

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
  ShapesDialog(DDS::DomainParticipant_var participant, const std::string& partition);
  virtual ~ShapesDialog();

public slots:
  virtual void onPublishButtonClicked();
  virtual void onSubscribeButtonClicked();
  virtual void onReaderQosButtonClicked();
  virtual void onWriterQosButtonClicked();
  virtual void onFilterButtonClicked();

private:
  ShapesDialog(const ShapesDialog& orig);

private:
  Ui::ShapesDialog  mainWidget;
  ShapesWidget*     shapesWidget;
  ReaderQosDialog   readerQos_;
  WriterQosDialog   writerQos_;
  FilterDialog*     filterDialog_;

  QTimer                     timer;
  DDS::Topic_var             circleTopic_;
  DDS::Topic_var             squareTopic_;
  DDS::Topic_var             triangleTopic_;
  DDS::DomainParticipant_var participant_;
  DDS::Publisher_var         publisher_;
  DDS::Subscriber_var        subscriber_;
  std::string                filterExpression_;
  DDS::StringSeq             filterParams_;
};

#endif  /* _ISHAPESFORM_HPP */

