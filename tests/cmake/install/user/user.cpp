#include <MovingObject.h>

#ifdef __has_include
#  if __has_include(<secretC.h>)
#    error "It shouldn't be possible to include secretC.h!"
#  endif
#endif

int main(int argc, char* argv[])
{
  MovingObject earth = {0};
  earth.acc.x = 2.5;
  earth.acc.y = 0.1;
  for (int i = 0; i < 4; ++i) {
    update(earth, 3.0);
    print(earth, "Earth");
  }

  return 0;
}
