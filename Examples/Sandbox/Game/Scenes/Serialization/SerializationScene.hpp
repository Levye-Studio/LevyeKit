#pragma once
#include <Levye/Modules/Serialization/Serialization.hpp>
namespace Sandbox {
/** @brief Game-facing save example and persistent reload verification. */
struct SerializationScene {
  static void Initialize(Levye::DocumentHandle& document);
  static void AfterReload(Levye::DocumentHandle document);
  static void Shutdown(Levye::DocumentHandle& document);

 private:
  static void SavePlayerExample();
};
}  // namespace Sandbox
