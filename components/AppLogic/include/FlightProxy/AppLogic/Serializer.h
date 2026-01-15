#pragma once
#include "FlightProxy/Core/TypeSignature.h"
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace FlightProxy {
namespace AppLogic {

class Serializer {
public:
  // Serializa tipos simples (int, float, structs empaquetados)
  template <typename T>
  static typename std::enable_if<std::is_trivially_copyable_v<T>, void>::type
  serialize(const T &data, std::vector<uint8_t> &buffer) {
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&data);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
  }

  // Serializa Strings (Longitud + Contenido)
  static void serialize(const std::string &data, std::vector<uint8_t> &buffer) {
    uint32_t len = static_cast<uint32_t>(data.size());
    serialize(len, buffer);
    buffer.insert(buffer.end(), data.begin(), data.end());
  }

  // Deserializa tipos simples
  template <typename T>
  static typename std::enable_if<std::is_trivially_copyable_v<T>, void>::type
  deserialize(T &data, const std::vector<uint8_t> &buffer, size_t &offset) {
    std::memcpy(&data, &buffer[offset], sizeof(T));
    offset += sizeof(T);
  }

  // Deserializa Strings
  static void deserialize(std::string &data, const std::vector<uint8_t> &buffer,
                          size_t &offset) {
    uint32_t len;
    deserialize(len, buffer, offset);
    data.assign(reinterpret_cast<const char *>(&buffer[offset]), len);
    offset += len;
  }
};

} // namespace AppLogic
} // namespace FlightProxy
