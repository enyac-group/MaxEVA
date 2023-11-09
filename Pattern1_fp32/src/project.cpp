
#include <adf.h>
#include "kernels.h"
#include "project.h"

using namespace adf;

simpleGraph mygraph;

int main(void) {
  mygraph.init();
  mygraph.run(10);
  mygraph.end();
  return 0;
}
