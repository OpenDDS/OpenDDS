#ifndef _SHAPEDYNAMICS_HPP
#define _SHAPEDYNAMICS_HPP

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtCore/QRect>
#include <QtCore/QPoint>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <vector>

class ShapeDynamics {
public:
  ShapeDynamics(int x0, int y0);
  ShapeDynamics(int x0, int y0, const QRect& constraint);
  virtual ~ShapeDynamics();
public:
  virtual QPoint getPosition();

  virtual std::vector<QPoint> getPositionList();

  virtual void setConstraint(const QRect& rect);

  virtual void simulate() = 0;

private:
  ShapeDynamics(const ShapeDynamics& orig);

protected:
  QPoint pos_;
  std::vector<QPoint> plist_;
  QRect  constraint_;
};

#endif /* _SHAPEDYNAMICS_HPP */
