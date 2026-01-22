#pragma once
#include "AlmacenFlexible.h"

namespace FlightProxy::AppLogic {
enum DataIDs : DataID {
  ID_STATUS_Data = 0,
  ID_RC_Input = 1,
  ID_RC_Output = 2,
  ID_RC_InputNorm = 3,

  ID_IMU_Data = 10,
  ID_BARO_Data = 11,
  ID_GPS_Data = 12,

  // Ctrl level selector
  ID_CTRL_LVL = 19,
  // Ctrls lvl 2
  ID_CTRL_LATVEL_IN = 20,
  ID_CTRL_LATVEL_OUT = 21,
  ID_CTRL_FRNTVEL_IN = 22,
  ID_CTRL_FRNTVEL_OUT = 23,
  ID_CTRL_VERTVEL_IN = 24,
  ID_CTRL_VERTVEL_OUT = 25,
  ID_CTRL_ANGVEL_IN = 26,
  ID_CTRL_ANGVEL_OUT = 27,
  // Ctrls lvl3
  // No hay
  // Ctrls lvl4
  ID_CTRL_HORPOS_IN = 40,
  ID_CTRL_HORPOS_OUT = 41,
  ID_CTRL_VERTPOS_IN = 42,
  ID_CTRL_VERTPOS_OUT = 43,
};
}
