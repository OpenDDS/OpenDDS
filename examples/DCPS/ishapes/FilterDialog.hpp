#ifndef  DDS_DEMO_ISHAPES_FILTER_DIALOG_HPP_
#define  DDS_DEMO_ISHAPES_FILTER_DIALOG_HPP_

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtGui/QtGui>
#include <QtCore/QRect>
#include <ui_filterForm.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <ShapesWidget.hpp>

class FilterDialog : public QDialog {

  Q_OBJECT;

public:
    FilterDialog(ShapesWidget* widget);
    virtual ~FilterDialog();

public slots:
  virtual void accept();
  virtual void reject();
  virtual void updateX0(int x);
  virtual void updateY0(int y);
  virtual void updateX1(int w);
  virtual void updateY1(int h);
  virtual void updateFilterStatus(bool on);

  virtual bool isEnabled();

  virtual QRect getFilterBounds();
  virtual bool filterOutside();

private:
  Ui_FilterDialog filterDialog_;
  ShapesWidget* widget_;
  QRect filter_;
  bool enabled_;
};

#endif /* DDS_DEMO_ISHAPES_FILTER_DIALOG_HPP_ */
