
#ifndef __complex_H
#define __complex_H

__device__
const double2 operator-(const double2& vala, const double2& valb);

__device__
const double2 operator+(const double2& vala, const double2& valb);

__device__
const double2 operator*(const double& vala, const double2& valb);

__device__
const double2 operator*(const double2& vala, const double& valb);

__device__
double2 complex_exp(double2 val);

__device__
double pown(double val, unsigned long i);

__device__
double complex_mag2(double2 val);

__device__
double complex_mag(double2 val);

__device__
double2 complex_ln(double2 val);

__device__
double2 complex_mult(double2 vala, double2 valb);

__device__
double2 complex_pow(double2 val, double2 w);

__device__
double2 complex_pown(double2 val, unsigned long n);

__device__
double2 complex_divide(double2 vala, double2 valb);

#endif

