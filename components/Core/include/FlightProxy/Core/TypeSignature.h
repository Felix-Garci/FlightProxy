#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace FlightProxy {
namespace Core {

template <typename T> struct TypeSignature {
  static std::string Get() { return "unknown"; }
};

// Alfabeto de tipos básicos
template <> struct TypeSignature<int32_t> {
  static std::string Get() { return "i32"; }
};
template <> struct TypeSignature<uint32_t> {
  static std::string Get() { return "u32"; }
};
template <> struct TypeSignature<uint16_t> {
  static std::string Get() { return "u16"; }
};
template <> struct TypeSignature<uint8_t> {
  static std::string Get() { return "u8"; }
};
template <> struct TypeSignature<int16_t> {
  static std::string Get() { return "i16"; }
};
template <> struct TypeSignature<float> {
  static std::string Get() { return "f32"; }
};
template <> struct TypeSignature<double> {
  static std::string Get() { return "f64"; }
};
template <> struct TypeSignature<bool> {
  static std::string Get() { return "b"; }
};
template <> struct TypeSignature<std::string> {
  static std::string Get() { return "str"; }
};

template <typename T> struct TypeSignature<std::vector<T>> {
  static std::string Get() { return "vec<" + TypeSignature<T>::Get() + ">"; }
};

template <typename T, std::size_t N> struct TypeSignature<std::array<T, N>> {
  static std::string Get() {
    return "arr<" + TypeSignature<T>::Get() + "," + std::to_string(N) + ">";
  }
};

template <typename T> struct TypeFields {
  static std::string Get() { return "value"; }
};

} // namespace Core
} // namespace FlightProxy

// --- MACROS (Definidas fuera para evitar problemas de visibilidad) ---

#define AS_STRUCT_MEMBER(type, name) type name;
#define AS_SIG_MEMBER(type, name)                                              \
  +FlightProxy::Core::TypeSignature<type>::Get() + ","
#define AS_NAME_MEMBER(type, name) #name ","

#define REGISTER_FP_STRUCT(Name, FieldsMacro)                                  \
  namespace FlightProxy {                                                      \
  namespace Core {                                                             \
  struct Name {                                                                \
    FieldsMacro(AS_STRUCT_MEMBER)                                              \
  };                                                                           \
  }                                                                            \
  }                                                                            \
  template <>                                                                  \
  struct FlightProxy::Core::TypeSignature<FlightProxy::Core::Name> {           \
    static std::string Get() {                                                 \
      /* El std::string{} inicial evita el error de operador unario '+' */     \
      std::string fields = std::string {}                                      \
      FieldsMacro(AS_SIG_MEMBER);                                              \
      if (!fields.empty())                                                     \
        fields.pop_back();                                                     \
      return "s<" + fields + ">";                                              \
    }                                                                          \
  };                                                                           \
  /* Especialización para los NOMBRES de los campos */                         \
  template <> struct FlightProxy::Core::TypeFields<FlightProxy::Core::Name> {  \
    static std::string Get() {                                                 \
      /* Concatenación automática de literales de string en C++ */             \
      std::string names = "" FieldsMacro(AS_NAME_MEMBER);                      \
      if (!names.empty() && names.back() == ',')                               \
        names.pop_back();                                                      \
      return names;                                                            \
    }                                                                          \
  };
