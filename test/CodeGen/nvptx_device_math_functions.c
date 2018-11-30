// Test calling of device math functions.
///==========================================================================///

// RUN: %clang -fmath-errno -S -emit-llvm -o - %s -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda | FileCheck -check-prefix CHECK-YES %s

void test_sqrt(double a1) {
  #pragma omp target
  {
    // CHECK-YES: call double @llvm.nvvm.sqrt.rn.d(double
    double l1 = sqrt(a1);
  }
}

void test_pow(float a0, double a1, long double a2) {
  #pragma omp target
  {
    // CHECK-YES: call double @__internal_accurate_pow(double
    double l1 = pow(a1, a1);
  }
}
