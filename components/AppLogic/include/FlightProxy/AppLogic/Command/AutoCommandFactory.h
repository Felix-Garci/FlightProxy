#pragma once
#include "AutoCommand.h"
#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/AppLogic/Command/CommandManager.h"
#include "FlightProxy/AppLogic/Serializer.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Command {

template <typename PacketT> class AutoCommandFactory {
public:
  AutoCommandFactory(std::weak_ptr<AlmacenFlexible> bb)
      : bb_(bb), s_(std::make_shared<Serializer>()) {}

  ~AutoCommandFactory() {}

  template <typename T>
  void produceCMD(uint32_t id, bool on_consumidor, bool on_productor,
                  std::string customName = "") {
    std::function<T(void)> consumidor;
    std::function<void(T)> productor;

    if (!bb_.expired()) {
      auto bb = bb_.lock();
      consumidor = bb->registrarConsumidor<T>(id);
      productor = bb->registrarProductor<T>(id);
    }

    // vamos a meter en las lambdas una copia del ptro a serializer
    // aunque la factoria se destrulla, el serializer seguira vivo en las
    // lambdas
    auto serializer = this->s_;

    uint32_t she_id = 100 + id;
    AutoComandHandler she_handler = [serializer, on_consumidor, on_productor](
                                        const std::vector<uint8_t> &) {
      std::vector<uint8_t> buffer;
      std::string sig = Core::TypeSignature<T>::Get();
      sig += (on_consumidor ? "1" : "0");
      sig += (on_productor ? "1" : "0");

      serializer->serialize(sig, buffer);
      return buffer;
    };

    uint32_t nme_id = 200 + id;
    AutoComandHandler nme_handler = [serializer,
                                     customName](const std::vector<uint8_t> &) {
      std::vector<uint8_t> buffer;
      std::string name = Core::TypeFields<T>::Get();
      if (name == "value" && !customName.empty()) {
        name = customName;
      }
      serializer->serialize(name, buffer);
      return buffer;
    };

    // rellenamos los handlers;
    cmds_[she_id] = std::make_shared<AutoCommand<PacketT>>(she_id, she_handler);
    cmds_[nme_id] = std::make_shared<AutoCommand<PacketT>>(nme_id, nme_handler);

    if (on_consumidor) {
      uint32_t get_id = 300 + id;
      AutoComandHandler get_handler =
          [serializer, consumidor](const std::vector<uint8_t> &) {
            std::vector<uint8_t> buffer;
            if (consumidor) {
              T data = consumidor();
              serializer->serialize(data, buffer);
            }
            return buffer;
          };
      cmds_[get_id] =
          std::make_shared<AutoCommand<PacketT>>(get_id, get_handler);
    }

    if (on_productor) {
      uint32_t set_id = 400 + id;
      AutoComandHandler set_handler =
          [serializer, productor](const std::vector<uint8_t> &payload) {
            if (!productor || payload.empty())
              return std::vector<uint8_t>{0x00};

            T data;
            size_t offset = 0;
            try {
              serializer->deserialize(data, payload, offset);
              productor(data);

              return std::vector<uint8_t>{0x01};
            } catch (...) {
              return std::vector<uint8_t>{0x00};
            }
          };
      cmds_[set_id] =
          std::make_shared<AutoCommand<PacketT>>(set_id, set_handler);
    }
  }

  void loadCMDS(std::shared_ptr<CommandManager<PacketT>> cmdMgr) {
    for (auto const &[id, cmd] : cmds_) {
      cmdMgr->registerCommand(cmd);
    }
    cmds_.clear();
  }

private:
  std::weak_ptr<AlmacenFlexible> bb_;
  std::shared_ptr<Serializer> s_;
  std::map<uint32_t, std::shared_ptr<ICommand<PacketT>>> cmds_;
};

} // namespace Command
} // namespace AppLogic
} // namespace FlightProxy
