#include "FlightProxy/AppLogic/Control/ControlMaster.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
ControlMaster::ControlMaster() {}

ControlMaster::~ControlMaster() {}

void ControlMaster::init(std::shared_ptr<AlmacenFlexible> bb) {
  inRcGetter_ = bb->registrarConsumidor<Core::RCData>(ID_RC_Input);
  outRcNormSetter_ = bb->registrarProductor<Core::RCNORMData>(ID_RC_InputNorm);
  outRcSetter_ = bb->registrarProductor<Core::RCData>(ID_RC_Output);

  this->control_.init(bb);
}

void ControlMaster::start() {
  Core::OSAL::TaskConfig config;
  config.name = "CtrMgr";
  config.stackSize = 4096;
  config.priority = 2;

  FP_LOG_D("ControlMaster", "iniciando");
  task_ = Core::OSAL::OSALFactory::createTask(
      [this]() { this->eventLoop(); }, // Lambda que llama al bucle
      config);

  if (task_) {
    isRunning_ = true;
    task_->start();
  }
}

void ControlMaster::stop() {
  if (isRunning_ && task_) {
    isRunning_ = false;
    task_->stop();
  }
}

void ControlMaster::eventLoop() {

  FP_LOG_D("ControlMaster", "dentro de event loop");
  Core::RCData rcData;
  Core::RCNORMData rcNormData;
  uint8_t ctrlLevel = 1;
  auto lastTime = Core::OSAL::OSALFactory::getSystemTimeMs();

  while (isRunning_) {
    auto currentTime = Core::OSAL::OSALFactory::getSystemTimeMs();
    float dt = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    rcData = inRcGetter_();
    rcNormData = this->normalizer_.norm(rcData);
    // FP_LOG_D("ControlMaster", "%d ,r: %.2f,p: %.2f,t: %.2f,y: %.2f",
    //          rcNormData.armed, rcNormData.roll, rcNormData.pitch,
    //          rcNormData.throttle, rcNormData.yaw);
    outRcNormSetter_(rcNormData);

    ctrlLevel = this->state_.step(rcNormData);
    // FP_LOG_D("ControlMaster", "Nivel de control: %d", ctrlLevel);
    rcNormData = this->control_.step(ctrlLevel, rcNormData, dt);

    rcData = this->normalizer_.de_norm(rcNormData);
    // FP_LOG_D("ControlMaster", "%d ,r: %d,p: %d,t: %d,y: %d", rcData.aux1,
    //          rcData.roll, rcData.pitch, rcData.throttle, rcData.yaw);

    outRcSetter_(rcData);

    auto elapsed = Core::OSAL::OSALFactory::getSystemTimeMs() - lastTime;
    int32_t sleep_ms = (int32_t)this->periodCicleMS_ - (int32_t)elapsed;
    if (sleep_ms < 0)
      sleep_ms = 0;

    Core::OSAL::OSALFactory::sleep(sleep_ms);
  }
  FP_LOG_E("ControlMaster", "salimos de event loop");
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
