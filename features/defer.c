#include <stdint.h>
#include <stdlib.h>
extern int open(const char *filename, int mode);
extern int close(int fd);

/***
func test():void {
    const fd = open("test",0);
    defer {
        close(fd);
    }
    {
        // do something
        return;
    }
    {
        // do something
        return;
    }
}
*/

void test() {
  uint64_t __back = 0;
  int fd = open("test", 0);
  {
    // do something
    __back = 1;
    goto _defer_0;
  _back_1:
    return;
  }
  {
    // do something
    __back = 2;
    goto _defer_0;
  _back_2:
    return;
  }
  return;
_defer_0:
  close(fd);
  goto _back;
_back:
  switch (__back) {
  case 1:
    goto _back_1;
  case 2:
    goto _back_2;
  default:
    abort();
  }
}