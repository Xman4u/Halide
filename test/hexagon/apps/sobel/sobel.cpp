#include <Halide.h>
#include "halide-hexagon-setup.h"
#include <stdio.h>
using namespace Halide;
#define COMPILE_OBJ(X)  ((X).compile_to_file("sobel", args, target))

/*
 * Two sets of coefficients:
 * horizontal:
 * [  1  2  1 ]
 * [  0  0  0 ]
 * [ -1 -2 -1 ]
 * vertical:
 * [  1  0 -1 ]
 * [  2  0 -2 ]
 * [  1  0 -1 ]
 * Convolve image with each, and add results together.
 * Clamp back to 8 bits
 */

void test_sobel(Target &target) {

  Halide::Var x("x"), y("y");
  // Halide:: input
  ImageParam input(type_of<uint8_t>(), 2);
  // Halide:: Function
  Halide::Func input_16("input_16");
  input_16(x, y) = cast<uint16_t>(input(x, y));

  Halide::Func sobel_x_avg("sobel_x_avg");
  sobel_x_avg(x,y) = input_16(x-1, y)  + input_16(x+1,y) + 2*input_16(x, y);
  Halide::Func sobel_x("sobel_x");
  sobel_x(x, y) = abs(sobel_x_avg(x, y-1) - sobel_x_avg(x, y+1));

  Halide::Func sobel_y_avg("sobel_y_avg");
  sobel_y_avg(x,y) = input_16(x, y-1) + 2*input_16(x, y)  + input_16(x, y+1);
  Halide::Func sobel_y("sobel_y");
  sobel_y(x, y) = abs(sobel_y_avg(x-1, y) - sobel_y_avg(x+1, y));

  Halide::Func Sobel("Sobel");
  Sobel(x, y) = cast<uint8_t>(clamp(sobel_y(x, y) + sobel_x(x, y), 0, 255));

  // Halide:: Schedule
  Sobel.vectorize(x, 1<<LOG2VLEN);
  std::vector<Argument> args(1);
  args[0]  = input;
#ifdef BITCODE
  Sobel.compile_to_bitcode("sobel.bc", args, target);
#endif
#ifdef ASSEMBLY
  Sobel.compile_to_assembly("sobel.s", args, target);
#endif
#ifdef STMT
  Sobel.compile_to_lowered_stmt("sobel.html", args, HTML);
#endif
#ifdef RUN
  COMPILE_OBJ(Sobel);
#endif
}
int main(int argc, char **argv) {
  Target target;
  setupHexagonTarget(target);
#if LOG2VLEN == 7
  target.set_feature(Target::HVX_DOUBLE);
#endif
  test_sobel(target);
  printf ("Done\n");
  return 0;
}

