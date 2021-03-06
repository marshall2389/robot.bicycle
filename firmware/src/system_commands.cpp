#include <cstdint>
#include <cstring>
#include "system_commands.h"
#include "motor_controller.h"

void SystemCommands::disable_controllers(BaseSequentialStream*, int, char**)
{
  using namespace hardware;
  if (instances[rear_wheel])
    instances[rear_wheel]->disable();
  if (instances[fork])
    instances[fork]->disable();
}

void SystemCommands::reset(BaseSequentialStream*, int, char**)
{
  NVIC_SystemReset();
}

