#include "SoftwareControlSetOrig.h"
#define RADAR_STATE_NAMES { \
    _("RADAR_OFF"), \
    _("RADAR_STANDBY"), \
    _("RADAR_WARMING_UP"), \
    _("RADAR_TIMED_IDLE"), \
    _("RADAR_STOPPING"), \
    _("RADAR_SPINNING_DOWN"), \
    _("RADAR_STARTING"), \
    _("RADAR_SPINNING_UP"), \
    _("RADAR_TRANSMIT") }
HAVE_CONTROL(CT_STATE, CTD_AUTO_NO, CTD_DEF_ZERO, 0, 8,
             CTD_STEP_1, RADAR_STATE_NAMES)
