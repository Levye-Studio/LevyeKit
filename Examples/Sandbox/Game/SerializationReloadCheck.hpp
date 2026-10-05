#pragma once

#include <Levye/Modules/Serialization/Serialization.hpp>
#include <string>

/** @brief Sandbox checks using only the game-facing serialization facade. */
namespace SandboxSerialization {
using Levye::DocumentHandle;
using Levye::Serialization;

inline bool Verify(DocumentHandle document, std::int64_t reloads) {
  const bool valid =
      Serialization::Contains(document) &&
      Serialization::GetInt(document, "player.score", -1) == 250 + reloads &&
      Serialization::GetFloat(document, "player.energy", -1) == 80.5 &&
      Serialization::GetBool(document, "player.alive", false) &&
      Serialization::GetString(document, "player.name") == "Sandbox" &&
      Serialization::GetInt(document, "reload.count", -1) == reloads &&
      Serialization::GetArraySize(document, "history") ==
          static_cast<std::uint64_t>(reloads + 1) &&
      Serialization::GetInt(document, "history[0]", -1) == 250 &&
      Serialization::GetInt(
          document, ("history[" + std::to_string(reloads) + "]").c_str(), -1) ==
          250 + reloads &&
      Serialization::GetArraySize(document, "objects") == 1 &&
      Serialization::GetString(document, "objects[0].name") == "LUCA" &&
      Serialization::GetArraySize(document, "objects[0].values") == 3 &&
      Serialization::GetFloat(document, "objects[0].values[0]", -1) == 1.5 &&
      Serialization::GetBool(document, "objects[0].values[1]", true) == false &&
      Serialization::GetString(document, "objects[0].values[2]") == "kept";
  if (!valid) return false;
  for (std::int64_t i = 0; i <= reloads; ++i) {
    const auto path = "history[" + std::to_string(i) + "]";
    if (Serialization::GetInt(document, path.c_str(), -1) != 250 + i)
      return false;
  }
  return true;
}

/** @brief Creates once at startup; failure releases the incomplete document. */
inline bool Initialize(DocumentHandle& document) {
  if (document != Levye::InvalidDocumentHandle) return false;
  document = Serialization::Create();
  if (document == Levye::InvalidDocumentHandle) return false;
  const bool written =
      Serialization::SetInt(document, "player.score", 250) &&
      Serialization::SetFloat(document, "player.energy", 80.5) &&
      Serialization::SetBool(document, "player.alive", true) &&
      Serialization::SetString(document, "player.name", "Sandbox") &&
      Serialization::SetInt(document, "reload.count", 0) &&
      Serialization::CreateArray(document, "history") &&
      Serialization::AppendInt(document, "history", 250) &&
      Serialization::CreateArray(document, "objects") &&
      Serialization::AppendObject(document, "objects") &&
      Serialization::SetString(document, "objects[0].name", "LUCA") &&
      Serialization::CreateArray(document, "objects[0].values") &&
      Serialization::AppendFloat(document, "objects[0].values", 1.5) &&
      Serialization::AppendBool(document, "objects[0].values", false) &&
      Serialization::AppendString(document, "objects[0].values", "kept");
  if (written && Verify(document, 0)) return true;
  Serialization::Destroy(document);
  document = Levye::InvalidDocumentHandle;
  return false;
}

/**
 * @brief Verifies the old handle, modifies it, and checks a save/load round
 * trip.
 *
 * Only the temporary loaded handle is destroyed here. The persistent document
 * stays owned by the host until normal game shutdown, including on failure.
 */
inline bool AfterReload(DocumentHandle document, const char* savePath) {
  const auto reloads = Serialization::GetInt(document, "reload.count", -1);
  if (reloads < 0 || !Verify(document, reloads)) return false;
  if (!Serialization::SetInt(document, "player.score", 251 + reloads) ||
      !Serialization::SetInt(document, "reload.count", reloads + 1) ||
      !Serialization::AppendInt(document, "history", 251 + reloads) ||
      !Verify(document, reloads + 1) ||
      !Serialization::Save(document, savePath))
    return false;
  const auto loaded = Serialization::Load(savePath);
  if (loaded == Levye::InvalidDocumentHandle) return false;
  const bool passed = loaded != document && Verify(loaded, reloads + 1);
  const bool destroyed = Serialization::Destroy(loaded);
  return passed && destroyed && !Serialization::Contains(loaded);
}

/** @brief Called during normal shutdown, never during module replacement. */
inline bool Shutdown(DocumentHandle& document) {
  if (document == Levye::InvalidDocumentHandle) return true;
  const auto old = document;
  if (!Serialization::Destroy(old)) return false;
  document = Levye::InvalidDocumentHandle;
  return !Serialization::Contains(old);
}
}  // namespace SandboxSerialization
