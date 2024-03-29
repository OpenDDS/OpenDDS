
#ifndef _SHAPESWIDGET_HPP
#define _SHAPESWIDGET_HPP

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtWidgets/QWidget>
#include <QtCore/QRect>
#include <QtGui/QPixmap>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <vector>
#include <Shape.hpp>

class ShapesWidget  : public QWidget
{
  Q_OBJECT
  public:
  typedef std::vector<std::shared_ptr<Shape> > ShapeList;
  typedef std::vector<QRect> FilterList;
public:

  ShapesWidget(QWidget *parent = 0);
  virtual ~ShapesWidget();

public:
  void addFilter(const QRect& filter);
  void displayFilter(const QRect& currentFilter);

public slots:
  void nextAnimationFrame();
  void addShape(std::shared_ptr<Shape> shape);

protected:
  void paintEvent(QPaintEvent *event);

private:
  ShapesWidget(const ShapesWidget& orig);

private:
  ShapeList shapeList_;
  FilterList filterList_;
  QRect currentFilter_;
  bool showCurrentFilter_;
  QPixmap logo_;
};

#endif /* _SHAPESWIDGET_HPP */
