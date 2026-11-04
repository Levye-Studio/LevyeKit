#include "SerializationScene.hpp"

#include <Levye/LevyeKit.hpp>
#include <string>

#include "SerializationReloadCheck.hpp"
namespace Sandbox {
/**
 * @brief Small example of serializing game-owned data.
 *
 * The game defines which fields are serialized without depending on yaml-cpp.
 */
struct PlayerSave {
  std::string name = "Player";
  int score = 0;
  Vector2 position{};
};

template <typename Archive>
bool Serialize(Archive& archive, PlayerSave& player) {
  return archive.Field("name", player.name) && archive.Field("score", player.score) &&
         archive.Field("position", player.position);
}

/**
 * @brief Demonstrates the custom serialization API with a small player save.
 */
void SerializationScene::SavePlayerExample() {
  using namespace Levye;

  if (!Serialization::Available()) return;

  PlayerSave player;
  player.name = "Nesmy";
  player.score = 420;
  player.position = {640.0f, 360.0f};

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Player save example: document creation failed.");
    return;
  }

  const bool saved = Serialization::Write(document, "player", player) &&
                     Serialization::Save(document, "player.yaml");

  if (saved) {
    Log::Info("Player save example written.");
  } else {
    Log::Error("Player save example failed.");
  }

  Serialization::Destroy(document);
}

void SerializationScene::Initialize(Levye::DocumentHandle& document) {
  if (Levye::Serialization::Available()) {
    Levye::Log::Info("Serialization module available.");
    SavePlayerExample();
    if (SerializationReloadCheck::Initialize(document))
      Levye::Log::Info("Hot-reload test: document created with scalars and arrays.");
    else
      Levye::Log::Error("Hot-reload test: failed to initialize document.");
  } else {
    Levye::Log::Warning("Serialization module unavailable.");
  }
}
void SerializationScene::AfterReload(Levye::DocumentHandle document) {
  if (Levye::Serialization::Available()) {
    if (SerializationReloadCheck::AfterReload(document, "serialization-reload-test.yaml"))
      Levye::Log::Info("Hot-reload test passed: serialization state preserved. ");
    else
      Levye::Log::Error("Hot-reload test failed: serialization.");
  }
}
void SerializationScene::Shutdown(Levye::DocumentHandle& document) {
  if (!SerializationReloadCheck::Shutdown(document))
    Levye::Log::Error("Hot-reload test: failed to destroy persistent document.");
}
}  // namespace Sandbox
