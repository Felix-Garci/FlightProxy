#pragma once
#include "FlightProxy/AppLogic/DataNode/IHwAlignment.h"

#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
class IDataNodeBase {
public:
  virtual ~IDataNodeBase() = default;
  virtual void transact() = 0;
  std::shared_ptr<IHwAlignment> alineador;
};
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
