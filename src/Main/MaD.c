#include <stdint.h>
#include <propeller2.h>
#include "MaD.h"
#include "JSON.h"
#include "app_motion.h"
#include "app_notification.h"
#include "ControlSystem.h"
#include "Debug.h"
#include "dev_nvram.h"
#include "watchdog.h"
#include "dev_cogManager.h"

int _stdio_debug_lock;

void mad_startupNVRAM(int dev_nvram_lock)
{
  if (dev_nvram_lock == -1)
  {
    printf("WARNING NO LOCKS AVAILABLE!!!, EXITING PROGRAM\n");
    _waitms(1000);
    _reboot();
  }

  dev_nvram_init(dev_nvram_lock);
  if (dev_nvram_nosync_runUntilReady() == false)
  {
    printf("WARNING NVRAM INIT FAILED!!!, EXITING PROGRAM\n");
    _waitms(1000);
    _reboot();
  }
}

void mad_begin()
{
  // Create a lock for the debug output
  _stdio_debug_lock = _locknew();
  if (_stdio_debug_lock == -1)
  {
    printf("WARNING NO LOCKS AVAILABLE!!!, EXITING PROGRAM\n");
    _waitms(1000);
    _reboot();
  }

  // Start up nvram before everything else
  int criticalLock = _locknew();
  if (criticalLock == -1)
  {
    printf("WARNING NO LOCKS AVAILABLE!!!, EXITING PROGRAM\n");
    _waitms(1000);
    _reboot();
  }
  mad_startupNVRAM(criticalLock); // start the non-volatile memory system

  watchdog_init(criticalLock);
  machine_state_init();
  dev_cogManager_init(criticalLock);

  state_machine_set(PARAM_SELF_CHARGE_PUMP, true);
  app_notification_send(APP_NOTIFICATION_TYPE_SUCCESS, "Device is alive!\n");

  // should replace with a cog manager, for watchdog, locks etc.
  while (true)
  {
    dev_nvram_run();
    dev_cogManager_run();

    watchdog_kick(WATCHDOG_CHANNEL_MONITOR);
    watchdog_run();
  }
}
