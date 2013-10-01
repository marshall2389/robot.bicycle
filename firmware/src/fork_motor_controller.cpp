#include <cmath>
#include <cstdint>
#include <cstdlib>
#include "control_loop.h"
#include "constants.h"
#include "fork_motor_controller.h"
#include "MPU6050.h"
#include "SystemState.h"

namespace hardware {

const uint8_t ccr_channel = 2;                 // PWM Channel 2
const float max_current = 6.0f;                // Copley Controls ACJ-055-18
const float torque_constant = 106.459f * constants::Nm_per_ozfin;
const float max_steer_angle = 45.0f * constants::rad_per_degree;
const float max_yaw_rate_command = 2.0f;       // rad/s
const float min_yaw_rate_command = -max_yaw_rate_command;

ForkMotorController::ForkMotorController()
  : MotorController("Fork"),
  e_(STM32_TIM3, constants::fork_counts_per_revolution),
  m_(GPIOF, GPIOF_STEER_DIR, GPIOF_STEER_ENABLE, GPIOF_STEER_FAULT,
     STM32_TIM1, ccr_channel, max_current, torque_constant),
  derivative_filter_{0, 10*2*constants::pi,
                     10*2*constants::pi, constants::loop_period_s},
  yaw_rate_command_{0.0f},
  estimation_threshold_{-1.0f / constants::wheel_radius},
  estimation_triggered_{false},
  control_triggered_{false},
  disturb_triggered_{false},
  control_delay_{10u},
  disturb_A_{0.0f}, disturb_f_{0.0f}
{
  instances[fork] = this;
}

ForkMotorController::~ForkMotorController()
{
  instances[fork] = 0;
}

bool ForkMotorController::set_reference(float yaw_rate)
{
  if (yaw_rate >= min_yaw_rate_command && yaw_rate <= max_yaw_rate_command) {
    yaw_rate_command_ = yaw_rate;
    return true;
  }
  return false;
}

void ForkMotorController::set_estimation_threshold(float speed)
{
  estimation_threshold_  = speed / -constants::wheel_radius;
}

void ForkMotorController::set_control_delay(uint32_t N)
{
  control_delay_= N;
}

void ForkMotorController::set_sinusoidal_disturbance(float A, float f)
{
  disturb_A_ = A;
  disturb_f_ = f;
}

void ForkMotorController::disable()
{
  m_.disable();
}

void ForkMotorController::enable()
{
  m_.enable();
}

void ForkMotorController::set_estimation_threshold_shell(BaseSequentialStream *chp,
                                                         int argc, char *argv[])
{
  if (argc == 1) {
      ForkMotorController* fmc = reinterpret_cast<ForkMotorController*>(instances[fork]);
      if (fmc) {
        fmc->set_estimation_threshold(tofloat(argv[0]));
        chprintf(chp, "%s estimation threshold set to %f.\r\n", fmc->name(),
                 fmc->estimation_threshold_);
      } else {
        chprintf(chp, "Enable collection before setting estimation threshold.\r\n");
      }
  } else {
    chprintf(chp, "Invalid usage.\r\n");
  }
}

void ForkMotorController::set_control_delay_shell(BaseSequentialStream *chp,
                                                  int argc, char *argv[])
{
  if (argc == 1) {
      ForkMotorController* fmc = reinterpret_cast<ForkMotorController*>(instances[fork]);
      if (fmc) {
        uint32_t N = std::atoi(argv[0]);
        fmc->set_control_delay(N);
        chprintf(chp, "%s control delay set to begin %u samples after estimation.\r\n", fmc->name(),
                 fmc->control_delay_);
      } else {
        chprintf(chp, "Enable collection before setting control threshold.\r\n");
      }
  } else {
    chprintf(chp, "Invalid usage.\r\n");
  }
}

void ForkMotorController::set_thresholds_shell(BaseSequentialStream *chp,
                                               int argc, char *argv[])
{
  if (argc == 2) {
    ForkMotorController* fmc = reinterpret_cast<ForkMotorController*>(instances[fork]);
    fmc->set_estimation_threshold(tofloat(argv[0]));
    uint32_t N = std::atoi(argv[0]);
    fmc->set_control_delay(N);
    chprintf(chp, "%s estimation threshold set to %f.\r\n", fmc->name(),
             fmc->estimation_threshold_);
    chprintf(chp, "%s control delay set to begin %u samples after estimation.\r\n",
             fmc->name(), fmc->control_delay_);
  } else {
    chprintf(chp, "Invalid usage.\r\n");
  }
}

void ForkMotorController::disturb_shell(BaseSequentialStream *chp,
                                        int argc, char *argv[])
{
  if (argc == 2) {
    ForkMotorController* fmc = reinterpret_cast<ForkMotorController*>(instances[fork]);
    float A = tofloat(argv[0]) * 0.01f;
    float f = tofloat(argv[1]);
    fmc->set_sinusoidal_disturbance(A, f);
    chprintf(chp, "Sinusoidal disturbance enabled.\r\n");
    chprintf(chp, "A = %f, f = %f.\r\n", A, f);
  } else {
    chprintf(chp, "Invalid usage.\r\n");
  }
}

float ForkMotorController::sinusoidal_disturbance_torque(const Sample& s) const {
  return disturb_A_ * std::sin(constants::two_pi * disturb_f_ * (s.system_time
        - disturb_t0_) * constants::system_timer_seconds_per_count);
}

void ForkMotorController::update(Sample & s)
{
  s.encoder.steer = e_.get_angle();
  s.encoder.steer_rate = derivative_filter_.output(s.encoder.steer);
  derivative_filter_.update(s.encoder.steer); // update for next iteration

  s.set_point.yaw_rate = yaw_rate_command_;

  s.motor_torque.desired_steer = 0.0f;
  if (should_estimate(s) && fork_control_.set_sample(s)) {
    float torque = fork_control_.compute_updated_torque(m_.get_torque());
    if (should_control(s)) {
      if (should_disturb(s) && s.bike_state == BikeState::RUNNING)
        torque += sinusoidal_disturbance_torque(s);
      s.motor_torque.desired_steer = torque;
      m_.set_torque(torque);
    } else {
      m_.set_torque(0.0f);
    }
  } else {
    m_.set_torque(0.0f);
  }
  s.motor_torque.steer = m_.get_torque();

  if (e_.rotation_direction())
    s.system_state |= systemstate::SteerEncoderDir;
  if (m_.is_enabled())
    s.system_state |= systemstate::SteerMotorEnable;
  if (m_.has_fault())
    s.system_state |= systemstate::SteerMotorFault;
  if (m_.current_direction())
    s.system_state |= systemstate::SteerMotorCurrentDir;
  s.threshold.estimation = estimation_threshold_;
  s.threshold.control = 0.0f; //  control_threshold_;
  s.has_threshold = true;
}

// estimation/control thresholds are in terms of wheel rate, which is defined
// to be negative when the speed of the bicycle is positive. estimation/control
// should occur when speed > threshold which is equivalent to rate < threshold.
bool ForkMotorController::should_estimate(const Sample& s)
{
  if (!estimation_triggered_) {
    estimation_triggered_ = s.encoder.rear_wheel_rate < estimation_threshold_;
    fork_control_.set_state(0.0f, s.encoder.steer,
                            s.mpu6050.gyroscope_x, s.encoder.steer_rate);
  }
  return estimation_triggered_;
}

bool ForkMotorController::should_control(const Sample& s)
{
  if (!control_triggered_)
    control_triggered_ = --control_delay_ == 0;
  return control_triggered_ && std::abs(s.encoder.steer) < max_steer_angle;
}

bool ForkMotorController::should_disturb(const Sample& s)
{
  if (!disturb_triggered_) {
    bool at_ref_speed = std::abs(s.encoder.rear_wheel_rate - s.set_point.theta_R_dot) <
                        std::abs(0.05f * s.set_point.theta_R_dot);
    const float norm = std::sqrt(std::pow(s.estimate.lean, 2.0f) +
                                 std::pow(s.estimate.steer, 2.0f) +
                                 std::pow(s.estimate.lean_rate, 2.0f) +
                                 std::pow(s.estimate.steer_rate, 2.0f));
    disturb_triggered_ = s.bike_state == BikeState::RUNNING && norm < 0.5f && at_ref_speed;
    disturb_t0_ = s.system_time;
  }
  return disturb_triggered_;
}

} // namespace hardware
