#include <MovingObject.h>
#include <secretC.h>

Vector add(Vector a, Vector b)
{
  Vector rv;
  rv.x = a.x + b.x;
  rv.y = a.y + b.y;
  return rv;
}

Vector update(MovingObject& object, CORBA::Long time)
{
  return object.pos;
}
