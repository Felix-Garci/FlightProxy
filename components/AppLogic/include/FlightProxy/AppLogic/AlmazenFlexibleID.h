#pragma once
#include "AlmacenFlexible.h"

namespace FlightProxy::AppLogic {
enum DataIDs : DataID {
  ID_STATUS_Data = 0,
  ID_RC_Input = 1,
  ID_RC_Output = 2,
  ID_IMU_Data = 10,
  ID_BARO_Data = 11,
  ID_ACTIVE_CTR = 12,      // 100,
  ID_SAMPPERIOID_CTR = 13, // 101,
  ID_PIDVALS_CTR = 14,     // 102,
  ID_PIDCST_CTR = 15,      // 103,
  ID_HOVER = 16,           // 104,
};
}
