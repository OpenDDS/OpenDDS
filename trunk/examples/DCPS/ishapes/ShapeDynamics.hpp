#ifndef _SHAPEDYNAMICS_HPP
#define _SHAPEDYNAMICS_HPP

#include <QtCore/QRect>
#include <QtCore/QPoint>
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

