#include <cstring>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "ff.h"

#include "MPU6050.h"
#include "SampleAndControl.h"
#include "SampleBuffer.h"
#include "SpeedController.h"
#include "YawRateController.h"

SampleAndControl::SampleAndControl()
  : timers({{0, 0, 0}}), Control_tp_(0), Enabled_(false)
{
  setFilename("samples.dat");
}

void SampleAndControl::Control_(__attribute__((unused))void * arg)
{
  chRegSetThreadName("Control");
  SampleAndControl::Instance().Control();
}

void SampleAndControl::Control()
{
  MPU6050 & imu = MPU6050::Instance();
  imu.Initialize(&I2CD2);
  SpeedController & speedControl = SpeedController::Instance();
  YawRateController & yawControl = YawRateController::Instance();
  SampleBuffer & sb = SampleBuffer::Instance();

  systime_t time = chTimeNow();     // Initial time

  for (uint32_t i = 0; !chThdShouldTerminate(); ++i) {
    time += MS2ST(5);            // Next deadline
    palTogglePad(IOPORT6, GPIOF_TIMING_PIN); // Sanity check for loop timing

    Sample & s = sb.CurrentSample();

    s.SystemTime = chTimeNow();

    imu.Acquire(s);

    s.RearWheelAngle = STM32_TIM8->CNT;
    s.FrontWheelAngle = STM32_TIM4->CNT;
    s.SteerAngle = STM32_TIM3->CNT;

    s.RearWheelRate = timers.Clockticks[0];
    s.SteerRate = timers.Clockticks[1];
    s.FrontWheelRate = timers.Clockticks[2];

    s.RearWheelRate_sp = speedControl.SetPoint();
    s.YawRate_sp = 0.0; // need to implement yaw rate controller

    // Compute new speed control action if controller is enabled.
    if (speedControl.isEnabled() && ((i % 10) == 0))
      speedControl.Update(s);

    if (yawControl.isEnabled()) {
      yawControl.Update(s);
    }

    s.CCR_rw = STM32_TIM1->CCR[0];    // Save rear wheel duty
    s.CCR_steer = STM32_TIM1->CCR[1]; // Save steer duty

    s.SystemState = 0;

    ++sb;                   // Increment to the next sample

    // Go to sleep until next 5ms interval
    chThdSleepUntil(time);
  }
  sb.Flush();  // ensure all data is written to disk
  imu.DeInitialize();
  chThdExit(0);
}

void SampleAndControl::chshellcmd(BaseSequentialStream *chp, int argc, char *argv[])
{
  SampleAndControl::Instance().shellcmd(chp, argc, argv);
}

void SampleAndControl::shellcmd(BaseSequentialStream *chp, int argc, char *argv[])
{
  if (argc == 0) { // toggle enabled/disabled
    if (isEnabled()) {
      setEnabled(false);
      chprintf(chp, "Sample & Control thread disabled.\r\n");
    } else {
      setEnabled(true);
      chprintf(chp, "Sample & Control thread enabled.\r\n");
    }
  } else if (argc == 1) { // change filename
    if (isEnabled()) {
      setEnabled(false);
      chprintf(chp, "Sample & Control thread disabled.\r\n");
    }
    setFilename(argv[0]);
    chprintf(chp, "Filename changed to %s.\r\n", Filename_);
  }
  return;
}

/*
 * This function should:
 * 1) Open a FIL object in write mode with filename Filename_
 *   a) if unable, return without Enabling
 * 2) Get a reference to SampleBuffer call connectToFile(&f);
 *   a) file must be Open and Writable for this to work.  It is up to client of
 *      SampleBuffer to ensure this is the case, no error checking is
 *      performed.
 * 3) Start the control thread.
 */
void SampleAndControl::setEnabled(bool state)
{
  if (state) {
    if (f_open(&f_, Filename_, FA_CREATE_ALWAYS | FA_WRITE)) return;

    SampleBuffer::Instance().File(&f_);

    nvicEnableVector(TIM5_IRQn, CORTEX_PRIORITY_MASK(7));

    Control_tp_ = chThdCreateStatic(SampleAndControl::waControlThread,
                                    sizeof(waControlThread),
                                    NORMALPRIO, (tfunc_t) Control_, 0);

    Enabled_ = true;
  } else {
    SpeedController::Instance().setEnabled(false);
    YawRateController::Instance().setEnabled(false);
    chThdTerminate(Control_tp_);
    chThdWait(Control_tp_);
    Control_tp_ = 0;
    nvicDisableVector(TIM5_IRQn);
    Enabled_ = false;
  }
}

void SampleAndControl::setFilename(const char * name)
{
  std::strcpy(Filename_, name);
}
