#include <MovingObject.h>
#include <secretC.h> // Just make sure we can reference this

#include <cstdio>

Vector add(Vector a, Vector b)
{
  Vector rv;
  rv.x = a.x + b.x;
  rv.y = a.y + b.y;
  return rv;
}

Vector multipy(Vector a, double by)
{
  Vector rv;
  rv.x = a.x * by;
  rv.y = a.y * by;
  return rv;
}

void update(MovingObject& object, double time)
{
  const Vector vel_change = multipy(object.acc, time);
  const Vector new_vel = add(object.vel, vel_change);
  object.pos = add(object.pos, add(multipy(object.vel, time), multipy(vel_change, time / 2)));
  object.vel = new_vel;
}

void print(const MovingObject& object, const char* name)
{
  printf("%s is at %f %f\n", name, object.pos.x, object.pos.y);
}
