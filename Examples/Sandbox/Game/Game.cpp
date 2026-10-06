#include <Levye/LevyeKit.hpp>
#include <Levye/Modules/Serialization/Serialization.hpp>
#include <Levye/Modules/Serialization/SerializationAPI.hpp>
#include <new>

#include "SerializationReloadCheck.hpp"

namespace {

/**
 * @brief Tests serialization through the public game-facing API.
 *
 * This test verifies that a reloadable game module can:
 * - Create a document owned by the host.
 * - Write values using nested paths.
 * - Save the document to disk.
 * - Load the saved document.
 * - Read its values back.
 * - Release both document handles.
 *
 * The game never accesses yaml-cpp directly.
 */
void TestSerialization() {
  using namespace Levye;

  if (!Serialization::Available()) {
    Log::Warning("Serialization module is unavailable.");
    return;
  }

  // ---------------------------------------------------------
  // 1. Create a document
  // ---------------------------------------------------------

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Failed to create serialization document.");
    return;
  }

  // ---------------------------------------------------------
  // 2. Write some values
  // ---------------------------------------------------------

  const bool written =
      Serialization::SetString(document, "player.name", "Nesmy") &&
      Serialization::SetInt(document, "player.score", 250) &&
      Serialization::SetFloat(document, "player.position.x", 640.0) &&
      Serialization::SetFloat(document, "player.position.y", 360.0) &&
      Serialization::SetBool(document, "player.alive", true);

  if (!written) {
    Log::Error("Failed to write serialization values.");
    Serialization::Destroy(document);
    return;
  }

  // ---------------------------------------------------------
  // 3. Save the document
  // ---------------------------------------------------------

  constexpr const char* savePath = "serialization-test.yaml";

  if (!Serialization::Save(document, savePath)) {
    Log::Error("Failed to save serialization document.");
    Serialization::Destroy(document);
    return;
  }

  Log::Info("Serialization document saved.");

  // ---------------------------------------------------------
  // 4. Load the document into a second handle
  // ---------------------------------------------------------

  const DocumentHandle loaded = Serialization::Load(savePath);

  if (loaded == InvalidDocumentHandle) {
    Log::Error("Failed to load serialization document.");
    Serialization::Destroy(document);
    return;
  }

  // ---------------------------------------------------------
  // 5. Verify the loaded values
  // ---------------------------------------------------------

  const std::string name = Serialization::GetString(loaded, "player.name");

  const std::int64_t score = Serialization::GetInt(loaded, "player.score", -1);

  const double x = Serialization::GetFloat(loaded, "player.position.x", -1.0);

  const double y = Serialization::GetFloat(loaded, "player.position.y", -1.0);

  const bool alive = Serialization::GetBool(loaded, "player.alive", false);

  const bool passed =
      name == "Nesmy" && score == 250 && x == 640.0 && y == 360.0 && alive;

  if (passed) {
    Log::Info("Serialization round-trip test passed.");
  } else {
    Log::Error("Serialization round-trip test failed.");
  }

  // ---------------------------------------------------------
  // 6. Release the host-owned documents
  // ---------------------------------------------------------

  Serialization::Destroy(loaded);
  Serialization::Destroy(document);
}

/**
 * @brief Tests how the public serialization API handles invalid input.
 *
 * These checks deliberately exercise failure cases:
 *
 * - Invalid document handles.
 * - Missing document paths.
 * - Empty document paths.
 * - Missing YAML files.
 * - Repeated document destruction.
 *
 * The game must remain running even when these operations fail.
 *
 * @return True when every failure case behaves as expected.
 */
bool TestSerializationErrors() {
  using namespace Levye;

  if (!Serialization::Available()) {
    Log::Warning("Serialization error test skipped: module unavailable.");
    return false;
  }

  bool passed = true;

  // ---------------------------------------------------------
  // 1. Invalid document handle
  // ---------------------------------------------------------

  // Reading from an invalid document should return the
  // caller-provided fallback value.
  const std::int64_t invalidValue =
      Serialization::GetInt(InvalidDocumentHandle, "player.score", -123);

  if (invalidValue != -123) {
    Log::Error("Serialization error test: invalid handle fallback failed.");
    passed = false;
  }

  // Writing to an invalid document should fail safely.
  if (Serialization::SetInt(InvalidDocumentHandle, "player.score", 100)) {
    Log::Error("Serialization error test: invalid handle accepted a write.");
    passed = false;
  }

  // ---------------------------------------------------------
  // 2. Create a valid document
  // ---------------------------------------------------------

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Serialization error test: could not create document.");
    return false;
  }

  // ---------------------------------------------------------
  // 3. Missing path
  // ---------------------------------------------------------

  // The document exists, but this property has never
  // been written. The supplied fallback should be returned.
  const std::int64_t missingValue =
      Serialization::GetInt(document, "player.missing", -456);

  if (missingValue != -456) {
    Log::Error("Serialization error test: missing path fallback failed.");
    passed = false;
  }

  // ---------------------------------------------------------
  // 4. Empty path
  // ---------------------------------------------------------

  // An empty path is not a valid destination for a value.
  if (Serialization::SetInt(document, "", 100)) {
    Log::Error("Serialization error test: empty path accepted.");
    passed = false;
  }

  // ---------------------------------------------------------
  // 5. Missing YAML file
  // ---------------------------------------------------------

  // This path should not exist. Load() should return
  // InvalidDocumentHandle rather than crashing.
  //
  // A deliberately nonexistent directory makes accidental
  // matches less likely.
  const DocumentHandle missingFile = Serialization::Load(
      "__levyekit_missing_serialization_test__/missing.yaml");

  if (missingFile != InvalidDocumentHandle) {
    Log::Error("Serialization error test: missing file was accepted.");

    // Avoid leaking a document if Load() unexpectedly succeeded.
    Serialization::Destroy(missingFile);
    passed = false;
  }

  // ---------------------------------------------------------
  // 6. Destroy a document twice
  // ---------------------------------------------------------

  const bool firstDestroy = Serialization::Destroy(document);

  const bool secondDestroy = Serialization::Destroy(document);

  // The first destruction should succeed.
  // The second should report that the document no longer exists.
  if (!firstDestroy || secondDestroy) {
    Log::Error("Serialization error test: repeated destruction failed.");
    passed = false;
  }

  // ---------------------------------------------------------
  // 7. Final result
  // ---------------------------------------------------------

  if (passed) {
    Log::Info("Serialization error tests passed.");
  } else {
    Log::Error("Serialization error tests failed.");
  }

  return passed;
}

/**
 * @brief Verifies Vector2 serialization through the public API.
 *
 * Tests:
 * - Writing both vector components.
 * - Reading them back.
 * - Reading a missing vector using a fallback.
 * - Rejecting an invalid document handle.
 */
void TestVector2Serialization() {
  using namespace Levye;

  if (!Serialization::Available()) {
    Log::Warning("Vector2 serialization test skipped.");
    return;
  }

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Vector2 test: document creation failed.");
    return;
  }

  // ---------------------------------------------------------
  // 1. Write a vector
  // ---------------------------------------------------------

  const Vector2 original{640.0f, 360.0f};

  const bool written =
      Serialization::SetVector2(document, "player.position", original);

  // ---------------------------------------------------------
  // 2. Read the vector
  // ---------------------------------------------------------

  const Vector2 loaded = Serialization::GetVector2(document, "player.position");

  const bool valuesMatch = loaded.x == original.x && loaded.y == original.y;

  // ---------------------------------------------------------
  // 3. Verify the fallback
  // ---------------------------------------------------------

  const Vector2 fallback{10.0f, 20.0f};

  const Vector2 missing =
      Serialization::GetVector2(document, "player.missingPosition", fallback);

  const bool fallbackWorks = missing.x == fallback.x && missing.y == fallback.y;

  // ---------------------------------------------------------
  // 4. Verify invalid-handle behavior
  // ---------------------------------------------------------

  const bool invalidWriteRejected = !Serialization::SetVector2(
      InvalidDocumentHandle, "player.position", original);

  // ---------------------------------------------------------
  // 5. Report results
  // ---------------------------------------------------------

  if (written && valuesMatch && fallbackWorks && invalidWriteRejected) {
    Log::Info("Vector2 serialization test passed.");
  } else {
    Log::Error("Vector2 serialization test failed.");
  }

  Serialization::Destroy(document);
}

/**
 * @brief Verifies Vector3 serialization through the public API.
 *
 * Tests:
 * - Writing a complete Vector3.
 * - Reading the original values.
 * - Fallback behavior for missing coordinates.
 * - Rejection of invalid document handles.
 */
void TestVector3Serialization() {
  using namespace Levye;

  if (!Serialization::Available()) {
    Log::Warning("Vector3 serialization test skipped.");
    return;
  }

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Vector3 test: document creation failed.");
    return;
  }

  // ---------------------------------------------------------
  // 1. Write a complete Vector3
  // ---------------------------------------------------------

  const Vector3 original{10.0f, 20.0f, 30.0f};

  const bool written =
      Serialization::SetVector3(document, "camera.position", original);

  // ---------------------------------------------------------
  // 2. Read the stored values
  // ---------------------------------------------------------

  const Vector3 loaded = Serialization::GetVector3(document, "camera.position");

  const bool valuesMatch = loaded.x == original.x && loaded.y == original.y &&
                           loaded.z == original.z;

  // ---------------------------------------------------------
  // 3. Test missing-coordinate fallback
  // ---------------------------------------------------------

  // Write only the x coordinate of a second vector.
  const bool partialWrite =
      Serialization::SetFloat(document, "camera.target.x", 42.0);

  const Vector3 fallback{1.0f, 2.0f, 3.0f};

  const Vector3 partial =
      Serialization::GetVector3(document, "camera.target", fallback);

  // x comes from the document.
  // y and z come from the fallback.
  const bool fallbackWorks = partialWrite && partial.x == 42.0f &&
                             partial.y == 2.0f && partial.z == 3.0f;

  // ---------------------------------------------------------
  // 4. Test invalid document handle
  // ---------------------------------------------------------

  const bool invalidWriteRejected = !Serialization::SetVector3(
      InvalidDocumentHandle, "camera.position", original);

  // ---------------------------------------------------------
  // 5. Report results
  // ---------------------------------------------------------

  if (written && valuesMatch && fallbackWorks && invalidWriteRejected) {
    Log::Info("Vector3 serialization test passed.");

  } else {
    Log::Error("Vector3 serialization test failed.");
  }

  Serialization::Destroy(document);
}

/**
 * @brief Tests array operations through the public game API.
 *
 * Verifies array creation, object appending, indexed
 * reads/writes, and out-of-range protection.
 */
void TestArraySerialization() {
  using namespace Levye;

  if (!Serialization::Available()) {
    Log::Warning("Array serialization test skipped.");
    return;
  }

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Array test: document creation failed.");
    return;
  }

  // Create an empty array.
  const bool created = Serialization::CreateArray(document, "organisms");

  // Append two objects.
  const bool firstAppended = Serialization::AppendObject(document, "organisms");

  const bool secondAppended =
      Serialization::AppendObject(document, "organisms");

  // Populate the first object.
  const bool firstWritten =
      Serialization::SetInt(document, "organisms[0].id", 1) &&
      Serialization::SetFloat(document, "organisms[0].energy", 80.0);

  // Populate the second object.
  const bool secondWritten =
      Serialization::SetInt(document, "organisms[1].id", 2) &&
      Serialization::SetFloat(document, "organisms[1].energy", 45.0);

  // Read the array size.
  const std::uint64_t count =
      Serialization::GetArraySize(document, "organisms");

  // Verify indexed reads.
  const bool valuesMatch =
      Serialization::GetInt(document, "organisms[0].id", -1) == 1 &&
      Serialization::GetInt(document, "organisms[1].id", -1) == 2 &&
      Serialization::GetFloat(document, "organisms[0].energy", -1.0) == 80.0 &&
      Serialization::GetFloat(document, "organisms[1].energy", -1.0) == 45.0;

  // An indexed write must not grow an array.
  const bool invalidWriteRejected =
      !Serialization::SetInt(document, "organisms[5].id", 5);

  if (created && firstAppended && secondAppended && firstWritten &&
      secondWritten && count == 2 && valuesMatch && invalidWriteRejected) {
    Log::Info("Array serialization test passed.");
  } else {
    Log::Error("Array serialization test failed.");
  }

  Serialization::Destroy(document);
}

/**
 * @brief Tests saving and loading nested YAML arrays.
 *
 * This test exercises the complete serialization pipeline:
 *
 * Game -> Public API -> Bridge -> Service -> YAML
 *
 * It also verifies that nested sequence indices remain
 * accessible after loading the document from disk.
 */
void TestArrayPersistence() {
  using namespace Levye;

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Array persistence: document creation failed.");
    return;
  }

  bool passed = true;

  // ---------------------------------------------------------
  // 1. Create an array of regions.
  // ---------------------------------------------------------

  passed &= Serialization::CreateArray(document, "world.regions");

  passed &= Serialization::AppendObject(document, "world.regions");

  // ---------------------------------------------------------
  // 2. Create an array inside the first region.
  // ---------------------------------------------------------

  passed &= Serialization::SetString(document, "world.regions[0].name",
                                     "Shallow Ocean");

  passed &= Serialization::CreateArray(document, "world.regions[0].organisms");

  passed &= Serialization::AppendObject(document, "world.regions[0].organisms");

  passed &= Serialization::AppendObject(document, "world.regions[0].organisms");

  // ---------------------------------------------------------
  // 3. Populate both organisms.
  // ---------------------------------------------------------

  passed &=
      Serialization::SetInt(document, "world.regions[0].organisms[0].id", 1);

  passed &= Serialization::SetFloat(
      document, "world.regions[0].organisms[0].energy", 80.0);

  passed &=
      Serialization::SetInt(document, "world.regions[0].organisms[1].id", 2);

  passed &= Serialization::SetFloat(
      document, "world.regions[0].organisms[1].energy", 45.0);

  // ---------------------------------------------------------
  // 4. Save the document.
  // ---------------------------------------------------------

  const std::string filePath = "array-persistence-test.yaml";

  passed &= Serialization::Save(document, filePath.c_str());

  // ---------------------------------------------------------
  // 5. Load the document into a new handle.
  // ---------------------------------------------------------

  const DocumentHandle loaded = Serialization::Load(filePath.c_str());

  if (loaded == InvalidDocumentHandle) {
    Log::Error("Array persistence: loading failed.");
    Serialization::Destroy(document);
    return;
  }

  // ---------------------------------------------------------
  // 6. Verify the loaded structure.
  // ---------------------------------------------------------

  passed &= Serialization::GetArraySize(loaded, "world.regions") == 1;

  passed &=
      Serialization::GetArraySize(loaded, "world.regions[0].organisms") == 2;

  passed &= Serialization::GetString(loaded, "world.regions[0].name") ==
            "Shallow Ocean";

  passed &= Serialization::GetInt(loaded, "world.regions[0].organisms[0].id",
                                  -1) == 1;

  passed &= Serialization::GetFloat(
                loaded, "world.regions[0].organisms[0].energy", -1.0) == 80.0;

  passed &= Serialization::GetInt(loaded, "world.regions[0].organisms[1].id",
                                  -1) == 2;

  passed &= Serialization::GetFloat(
                loaded, "world.regions[0].organisms[1].energy", -1.0) == 45.0;

  // ---------------------------------------------------------
  // 7. Reject an out-of-range indexed write.
  // ---------------------------------------------------------

  const bool invalidWriteAccepted =
      Serialization::SetInt(loaded, "world.regions[0].organisms[99].id", 99);

  passed &= !invalidWriteAccepted;

  passed &=
      Serialization::GetArraySize(loaded, "world.regions[0].organisms") == 2;

  // ---------------------------------------------------------
  // 8. Report and clean up.
  // ---------------------------------------------------------

  if (passed) {
    Log::Info("Array persistence test passed.");
  } else {
    Log::Error("Array persistence test failed.");
  }

  Serialization::Destroy(loaded);
  Serialization::Destroy(document);
}

/**
 * @brief Tests scalar array operations through the public API.
 *
 * Verifies that all four scalar types can be appended and
 * read through indexed serialization paths.
 */
void TestPublicScalarArrays() {
  using namespace Levye;

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Public scalar arrays: document creation failed.");
    return;
  }

  bool passed = true;

  // ---------------------------------------------------------
  // 1. Create the arrays.
  // ---------------------------------------------------------

  passed &= Serialization::CreateArray(document, "data.ids");
  passed &= Serialization::CreateArray(document, "data.temperatures");
  passed &= Serialization::CreateArray(document, "data.active");
  passed &= Serialization::CreateArray(document, "data.names");

  // ---------------------------------------------------------
  // 2. Append values.
  // ---------------------------------------------------------

  passed &= Serialization::AppendInt(document, "data.ids", 1);
  passed &= Serialization::AppendInt(document, "data.ids", 2);

  passed &= Serialization::AppendFloat(document, "data.temperatures", 18.5);

  passed &= Serialization::AppendFloat(document, "data.temperatures", 21.0);

  passed &= Serialization::AppendBool(document, "data.active", true);

  passed &= Serialization::AppendBool(document, "data.active", false);

  passed &= Serialization::AppendString(document, "data.names", "LUCA");

  passed &= Serialization::AppendString(document, "data.names", "Species-A");

  // ---------------------------------------------------------
  // 3. Verify the array sizes.
  // ---------------------------------------------------------

  passed &= Serialization::GetArraySize(document, "data.ids") == 2;

  passed &= Serialization::GetArraySize(document, "data.temperatures") == 2;

  passed &= Serialization::GetArraySize(document, "data.active") == 2;

  passed &= Serialization::GetArraySize(document, "data.names") == 2;

  // ---------------------------------------------------------
  // 4. Verify the stored values.
  // ---------------------------------------------------------

  passed &= Serialization::GetInt(document, "data.ids[0]", -1) == 1;

  passed &= Serialization::GetInt(document, "data.ids[1]", -1) == 2;

  passed &=
      Serialization::GetFloat(document, "data.temperatures[0]", -1.0) == 18.5;

  passed &=
      Serialization::GetFloat(document, "data.temperatures[1]", -1.0) == 21.0;

  passed &= Serialization::GetBool(document, "data.active[0]", false);

  passed &= !Serialization::GetBool(document, "data.active[1]", true);

  passed &= Serialization::GetString(document, "data.names[0]") == "LUCA";

  passed &= Serialization::GetString(document, "data.names[1]") == "Species-A";

  // ---------------------------------------------------------
  // 5. Verify invalid operations.
  // ---------------------------------------------------------

  passed &= !Serialization::AppendInt(InvalidDocumentHandle, "data.ids", 99);

  passed &= !Serialization::AppendString(document, "data.missing", "invalid");

  passed &= !Serialization::AppendBool(document, "data.ids[99]", true);

  // ---------------------------------------------------------
  // 6. Report the result.
  // ---------------------------------------------------------

  if (passed) {
    Log::Info("Public scalar array test passed.");
  } else {
    Log::Error("Public scalar array test failed.");
  }

  Serialization::Destroy(document);
}

/**
 * @brief Verifies array-type inspection through the public API.
 *
 * Tests:
 * - Existing empty arrays.
 * - Missing paths.
 * - Scalar values.
 * - Nested arrays.
 * - Invalid handles and paths.
 */
void TestPublicArrayTypeChecking() {
  using namespace Levye;

  const DocumentHandle document = Serialization::Create();

  if (document == InvalidDocumentHandle) {
    Log::Error("Array type test: document creation failed.");
    return;
  }

  bool passed = true;

  // ---------------------------------------------------------
  // 1. An empty array must be recognized.
  // ---------------------------------------------------------

  passed &= Serialization::CreateArray(document, "world.organisms");

  passed &= Serialization::IsArray(document, "world.organisms");

  passed &= Serialization::GetArraySize(document, "world.organisms") == 0;

  // ---------------------------------------------------------
  // 2. Missing paths must not be recognized as arrays.
  // ---------------------------------------------------------

  passed &= !Serialization::IsArray(document, "world.missing");

  // ---------------------------------------------------------
  // 3. Scalar values are not arrays.
  // ---------------------------------------------------------

  passed &= Serialization::SetInt(document, "world.population", 100);

  passed &= !Serialization::IsArray(document, "world.population");

  // ---------------------------------------------------------
  // 4. Nested arrays must be recognized.
  // ---------------------------------------------------------

  passed &= Serialization::AppendObject(document, "world.organisms");

  passed &= Serialization::CreateArray(document, "world.organisms[0].genes");

  passed &= Serialization::IsArray(document, "world.organisms[0].genes");

  passed &=
      Serialization::GetArraySize(document, "world.organisms[0].genes") == 0;

  // ---------------------------------------------------------
  // 5. Invalid arguments must be rejected.
  // ---------------------------------------------------------

  passed &= !Serialization::IsArray(InvalidDocumentHandle, "world.organisms");

  passed &= !Serialization::IsArray(document, "world.organisms[-1]");

  passed &= !Serialization::IsArray(document, "world.organisms[99]");

  passed &= !Serialization::IsArray(document, nullptr);

  // ---------------------------------------------------------
  // 6. Report and clean up.
  // ---------------------------------------------------------

  if (passed) {
    Log::Info("Public array type test passed.");
  } else {
    Log::Error("Public array type test failed.");
  }

  Serialization::Destroy(document);
}

struct GameState {
  Vector2 playerPosition{640.0f, 360.0f};

  float playerSpeed = 250.0f;

  int score = 1;

  Levye::AssetHandle playerTexture{};
  Levye::AssetHandle clickSound{};
  Levye::AssetHandle music{};
  // Handle to a serialization document owned by the host.
  //
  // Only the numeric handle lives in GameState. The YAML document itself
  // stays in SerializationService, which is not unloaded during hot reload.
  Levye::DocumentHandle persistentDocument = Levye::InvalidDocumentHandle;
};

void BindServices(const Levye::HostServices* services) {
  if (services)
    Levye::Services::Bind(services);
  else
    Levye::Services::Unbind();
}

GameState* GetState(void* state) { return static_cast<GameState*>(state); }

void InitializeState(void* state) { new (state) GameState{}; }

void DestroyState(void* state) { GetState(state)->~GameState(); }

void OnLoad(void* state) {
  GameState* game = GetState(state);
  // game->initialized = true;

  /*
   * Keyboard movement.
   *
   * Both WASD and arrow keys feed the same logical axes.
   */
  Levye::Input::BindKeyAxis("MoveX", KEY_A, KEY_D);

  Levye::Input::BindKeyAxis("MoveX", KEY_LEFT, KEY_RIGHT);

  Levye::Input::BindKeyAxis("MoveY", KEY_W, KEY_S);

  Levye::Input::BindKeyAxis("MoveY", KEY_UP, KEY_DOWN);

  /*
   * Controller movement.
   */
  Levye::Input::BindGamepadAxis("MoveX", 0, GAMEPAD_AXIS_LEFT_X);

  Levye::Input::BindGamepadAxis("MoveY", 0, GAMEPAD_AXIS_LEFT_Y);

  Levye::Input::BindKey("MoveUp", KEY_W);

  Levye::Input::BindKey("MoveUp", KEY_UP);

  Levye::Input::BindKey("MoveDown", KEY_S);

  Levye::Input::BindKey("MoveDown", KEY_DOWN);

  Levye::Input::BindKey("MoveLeft", KEY_A);

  Levye::Input::BindKey("MoveLeft", KEY_LEFT);

  Levye::Input::BindKey("MoveRight", KEY_D);

  Levye::Input::BindKey("MoveRight", KEY_RIGHT);

  Levye::Input::BindKey("Confirm", KEY_ENTER);
  Levye::Input::BindGamepadButton("Confirm", 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("Back", KEY_ESCAPE);
  Levye::Input::BindGamepadButton("Back", 0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

  Levye::Input::BindKey("TestSound", KEY_SPACE);
  Levye::Input::BindGamepadButton("TestSound", 0,
                                  GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

  Levye::Input::BindKey("PauseMusic", KEY_M);

  Levye::Input::BindKey("ResumeMusic", KEY_R);

  Levye::Input::BindKey("NormalTime", KEY_ONE);

  Levye::Input::BindKey("SlowTime", KEY_TWO);

  Levye::Input::BindKey("FastTime", KEY_THREE);

  Levye::Input::BindKey("Pause", KEY_P);

  Levye::Input::BindKey("TestMouse", KEY_C);
  Levye::Input::BindMouseButton("TestMouse", MOUSE_BUTTON_LEFT);

  game->playerTexture = Levye::Assets::LoadTexture("player.png");

  game->clickSound = Levye::Audio::LoadSound("click.wav");

  game->music = Levye::Audio::LoadMusic("music.ogg");

  Levye::Audio::SetMusicVolume(game->music, 0.5f);

  Levye::Audio::PlayMusic(game->music);

  if (Levye::Serialization::Available()) {
    Levye::Log::Info("Serialization module available.");

    // Existing save/load test.
    TestSerialization();
    TestSerializationErrors();
    TestVector2Serialization();
    TestVector3Serialization();
    TestArraySerialization();
    TestArrayPersistence();
    TestPublicScalarArrays();
    TestPublicArrayTypeChecking();

    // Persist only the handle; the host keeps the document across reloads.
    if (SandboxSerialization::Initialize(game->persistentDocument)) {
      Levye::Log::Info(
          "Hot-reload test: document created with scalars and arrays.");
    } else {
      Levye::Log::Error("Hot-reload test: failed to initialize document.");
    }
  } else {
    Levye::Log::Warning("Serialization module unavailable.");
  }

  Levye::Log::Info("Sandbox started.");
}

void OnAfterReload(void* state) {
  GameState* game = GetState(state);

  Levye::Log::Info("Sandbox reloaded.");

  // Optional modules may be disabled; that is not a reload failure.
  if (!Levye::Serialization::Available()) return;

  if (SandboxSerialization::AfterReload(game->persistentDocument,
                                        "serialization-reload-test.yaml")) {
    Levye::Log::Info(
        "Hot-reload test passed: same handle, values, arrays, modification and "
        "save/load.");
  } else {
    Levye::Log::Error(
        "Hot-reload test failed: persistence or save/load check.");
  }
}

void OnBeforeReload(void* state) {
  /*
   * This callback runs while the old game module is still loaded.
   *
   * Use it for temporary module-specific cleanup if needed, but do not
   * destroy persistent gameplay state or host-owned resources.
   */
  (void)state;

  Levye::Log::Info("Sandbox preparing for reload.");
}

void OnUpdate(void* state, float deltaTime) {
  GameState* game = GetState(state);

  if (Levye::Screen::Is("Menu")) {
    if (Levye::Input::IsPressed("Confirm")) {
      Levye::Screen::Set("Game");
    }

    return;
  }

  if (Levye::Input::IsPressed("TestMouse")) {
    Levye::Log::Info("Mouse pressed");
  }

  if (Levye::Input::IsReleased("TestMouse")) {
    Levye::Log::Info("Mouse released");
  }

  const float mouseWheel = Levye::Input::GetMouseWheel();
  const Vector2 mouseDelta = Levye::Input::GetMouseDelta();

  if (mouseWheel != 0.0f) {
    std::string wheelMSG = "Wheel: " + std::to_string(mouseWheel);
    Levye::Log::Info(wheelMSG.c_str());
  }

  if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
    std::string mouseMSG = "Mouse delta: x=" + std::to_string(mouseDelta.x) +
                           " y=" + std::to_string(mouseDelta.y);
    Levye::Log::Info(mouseMSG.c_str());
  }

  if (Levye::Screen::Is("Game")) {
    const float horizontal = Levye::Input::GetAxis("MoveX");
    const float vertical = Levye::Input::GetAxis("MoveY");

    game->playerPosition.x += horizontal * game->playerSpeed * deltaTime;
    game->playerPosition.y += vertical * game->playerSpeed * deltaTime;

    if (Levye::Input::IsPressed("Back")) {
      Levye::Screen::Set("Menu");

      return;
    }

    if (Levye::Input::IsPressed("NormalTime")) {
      Levye::Time::SetScale(1.0f);
    }

    if (Levye::Input::IsPressed("SlowTime")) {
      Levye::Time::SetScale(0.25f);
    }

    if (Levye::Input::IsPressed("FastTime")) {
      Levye::Time::SetScale(2.0f);
    }

    if (Levye::Input::IsPressed("Pause")) {
      const bool paused = Levye::Time::IsPaused();

      Levye::Time::SetPaused(!paused);
    }

    if (Levye::Input::IsPressed("PauseMusic")) {
      Levye::Audio::PauseMusic(game->music);
    }
    if (Levye::Input::IsPressed("ResumeMusic")) {
      Levye::Audio::ResumeMusic(game->music);
    }

    if (Levye::Input::IsPressed("TestSound")) {
      Levye::Audio::PlaySound(game->clickSound);
    }
  }
}

void OnFixedUpdate(void* state, float fixedDeltaTime) {
  constexpr float playerSpeed = 300.0f;
}

void OnDraw(void* state) {
  GameState* game = GetState(state);
  const Texture2D* playerTexture =
      Levye::Assets::GetTexture(game->playerTexture);

  const float alpha = Levye::Time::InterpolationAlpha();

  if (Levye::Screen::Is("Menu")) {
    ClearBackground(BLACK);

    DrawText("LEVYEKIT", 40, 40, 40, RAYWHITE);

    DrawText("Press ENTER / Controller A to play", 40, 100, 24, LIGHTGRAY);

    return;
  }

  if (Levye::Screen::Is("Game")) {
    ClearBackground(DARKBLUE);

    if (playerTexture) {
      DrawTexture(*playerTexture, static_cast<int>(game->playerPosition.x),
                  static_cast<int>(game->playerPosition.y), WHITE);
    } else {
      DrawCircleV(game->playerPosition, 30.0f, GOLD);
    }

    DrawText("Move: WASD / Arrows / Controller", 40, 40, 20, RAYWHITE);

    DrawText("ESC / Controller B: Menu", 40, 70, 20, LIGHTGRAY);
    DrawText("P: Pause time | M: Pause music | R: Resume music", 40, 100, 20,
             LIGHTGRAY);

    DrawText(Levye::Time::IsPaused() ? "PAUSED" : "RUNNING", 20, 20, 20, WHITE);
  }
}

void OnShutdown(void* state) {
  GameState* game = GetState(state);

  // OnShutdown runs when the application is actually exiting,
  // not when the game library is being hot-reloaded.
  //
  // This is the appropriate place to release the persistent
  // document created in OnLoad().
  if (!SandboxSerialization::Shutdown(game->persistentDocument)) {
    Levye::Log::Error(
        "Hot-reload test: failed to destroy persistent document.");
  }

  Levye::Log::Info("Sandbox shutting down.");
}
}  // namespace

/**
 * @brief Returns the public API implemented by the Sandbox game module.
 *
 * This is the single exported entry point required by LevyeKit.
 *
 * extern "C" disables C++ name mangling so the host can reliably locate
 * this function using the symbol name "GetGameAPI".
 */
LEVYE_GAME_EXPORT const Levye::GameAPI* GetGameAPI() {
  static const Levye::GameAPI api = {.version = Levye::GAME_API_VERSION,
                                     .BindServices = BindServices,

                                     .OnLoad = OnLoad,

                                     .OnBeforeReload = OnBeforeReload,
                                     .OnAfterReload = OnAfterReload,

                                     .OnUpdate = OnUpdate,
                                     .OnFixedUpdate = OnFixedUpdate,
                                     .OnDraw = OnDraw,
                                     .OnShutdown = OnShutdown,

                                     .InitializeState = InitializeState,
                                     .DestroyState = DestroyState,

                                     .stateSize = sizeof(GameState)};

  return &api;
}
