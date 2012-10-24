#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cmath>

template <class T>
class constants {
 public:
  // Standard engineering constants
  static constexpr T pi = 4.0*std::atan(1.0);
  static constexpr T pi_2 = 2.0*std::atan(1.0);
  static constexpr T pi_4 = std::atan(1.0);
  static constexpr T two_pi = 2.0*pi;
  static constexpr T rad_per_degree = pi / 180.0;
  static constexpr T e = std::exp(1.0);
  static constexpr T g = 9.81;

  // Time conversion
  static constexpr T Seconds_per_Systick = 0.001;

  // Gyroscope constants
  static constexpr T Gyroscope_temp_offset = 36.53;
  static constexpr T Gyroscope_temp_sensitivity = 1.0 / 340.0;
  static constexpr T Gyroscope_sensitivity = rad_per_degree / 131.0;

  // Accelerometer constants
  static constexpr T Accelerometer_sensitivity = g / 16384.0;

  // Steer angle
  static constexpr int Steer_CPR = 4000;
  static constexpr int Steer_quad_CPR = 4 * Steer_CPR;
  static constexpr T Steer_rad_per_count = two_pi / Steer_CPR;
  static constexpr T Steer_rad_per_quad_count = two_pi / Steer_quad_CPR;

  // Wheel rates and steer rate
  static constexpr int Wheel_CPR = 200;
  static constexpr int  Wheel_quad_CPR = 4 * Wheel_CPR;
  static constexpr T Wheel_rad_per_count = two_pi / Wheel_CPR;
  static constexpr T Wheel_rad_per_quad_count = two_pi / Wheel_quad_CPR;

  static constexpr T Rate_Timer_Frequency = 4.0e6;
  static constexpr T Wheel_rad_counts_per_sec = Rate_Timer_Frequency * Wheel_rad_per_count;
  static constexpr T Steer_rad_counts_per_sec = Rate_Timer_Frequency * Steer_rad_per_count;

  // Current command constants
  static constexpr T Current_max_rw = 12.0;     //  Copley ADP-090-36
  static constexpr T Current_max_steer = 6.0;   //  Copley ACJ-055-18
};

typedef constants<float> cf;
typedef constants<double> cd;
typedef constants<long double> cld;

#endif
