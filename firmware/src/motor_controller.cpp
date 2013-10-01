#include "ch.h"
#include "chprintf.h"
#include "gain_schedule.h"
#include "motor_controller.h"
#include "textutilities.h"

namespace hardware {

MotorController * instances[2] = {0, 0};

void MotorController::set_reference_shell(BaseSequentialStream *chp, int argc, char *argv[])
{
  if (argc == 0) {
      disable();
      chprintf(chp, "%s motor control disabled.\r\n", name());
  } else if (argc == 1) {
      if (set_reference(tofloat(argv[0]))) {
        enable();
        chprintf(chp, "%s motor control enabled and set.\r\n", name());
      } else {
        chprintf(chp, "Invalid input for %s motor control.\r\n", name());
      }
  } else {
    chprintf(chp, "Invalid usage.\r\n");
  }
}

} // namespace hardware

