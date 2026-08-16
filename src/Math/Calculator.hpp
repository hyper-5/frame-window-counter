#pragma once
#include <cmath>

void initErfcLUT();
double fast_erfc(double x);
void startGlobalRecalc();
void stopGlobalRecalc();