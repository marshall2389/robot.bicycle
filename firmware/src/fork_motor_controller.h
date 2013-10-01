#ifndef FORK_MOTOR_CONTROLLER_H
#define FORK_MOTOR_CONTROLLER_H
#include <array>
#include "ch.h"
#include "chprintf.h"
#include "filter.h"
#include "encoder.h"
#include "gain_schedule.h"
#include "motor.h"
#include "motor_controller.h"
#include "sample.pb.h"
#include "textutilities.h"

namespace hardware {

class ForkMotorController : public MotorController {
 public:
  ForkMotorController();
  ~ForkMotorController();
  virtual bool set_reference(float yaw_rate);
  virtual void disable();
  virtual void enable();
  virtual void update(Sample & s);

  static void set_estimation_threshold_shell(BaseSequentialStream * chp,
                                             int argc, char * argv[]);
  static void set_control_delay_shell(BaseSequentialStream * chp,
                                          int argc, char * argv[]);
  static void set_thresholds_shell(BaseSequentialStream * chp,
                                   int argc, char * argv[]);
  static void disturb_shell(BaseSequentialStream * chp,
                                int argc, char * argv[]);


 private:
  void set_estimation_threshold(float speed);
  void set_control_delay(uint32_t N);
  void set_sinusoidal_disturbance(float A, float f);
  float sinusoidal_disturbance_torque(const Sample& s) const;
  bool should_estimate(const Sample& s);
  bool should_control(const Sample& s);
  bool should_disturb(const Sample& s);

  Encoder e_;
  Motor m_;
  control::GainSchedule fork_control_;

  control::first_order_discrete_filter<float> derivative_filter_;
  float yaw_rate_command_;
  float estimation_threshold_; // in terms of rear wheel rate
  bool estimation_triggered_;
  bool control_triggered_;
  bool disturb_triggered_;
  uint32_t control_delay_;      // # number of sample periods
  float disturb_A_;
  float disturb_f_;
  uint32_t disturb_t0_;
};

} // namespace hardware

#endif // FORK_MOTOR_CONTROLLER_H

