#pragma once

#include <raylib.h>

#include <Levye/Core/ModuleAPI.hpp>
#include <Levye/Core/Services.hpp>
#include <Levye/Modules/Serialization/SerializationAPI.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Levye {

/**
 * @brief Public interface to the optional serialization module.
 *
 * This facade executes inside the reloadable game library.
 * Actual document storage and YAML processing remain in the host.
 *
 * Every operation checks whether the module is available.
 */
class Serialization {
 public:
  /**
   * @brief Checks whether the host provides the current serialization API
   * version.
   */
  [[nodiscard]] static bool Available() { return GetAPI() != nullptr; }

  /**
   * @brief Creates a host-owned document.
   */
  [[nodiscard]] static DocumentHandle Create(
      SerializationFormat format = SerializationFormat::YAML) {
    const auto* api = GetAPI();

    return api && api->Create ? api->Create(api->context, format)
                              : InvalidDocumentHandle;
  }

  /**
   * @brief Releases a host-owned document.
   */
  static bool Destroy(DocumentHandle handle) {
    const auto* api = GetAPI();

    return api && api->Destroy && api->Destroy(api->context, handle);
  }

  /**
   * @brief Checks whether the host still owns this document.
   */
  [[nodiscard]] static bool Contains(DocumentHandle handle) {
    const auto* api = GetAPI();

    return api && api->Contains && api->Contains(api->context, handle);
  }

  /**
   * @brief Writes an integer at a dotted document path.
   *
   * Example: "player.stats.score"
   */
  static bool SetInt(DocumentHandle handle, const char* path,
                     std::int64_t value) {
    const auto* api = GetAPI();

    return api && api->SetInt && api->SetInt(api->context, handle, path, value);
  }

  static bool SetFloat(DocumentHandle handle, const char* path, double value) {
    const auto* api = GetAPI();

    return api && api->SetFloat &&
           api->SetFloat(api->context, handle, path, value);
  }

  static bool SetBool(DocumentHandle handle, const char* path, bool value) {
    const auto* api = GetAPI();

    return api && api->SetBool &&
           api->SetBool(api->context, handle, path, value);
  }

  static bool SetString(DocumentHandle handle, const char* path,
                        const char* value) {
    const auto* api = GetAPI();

    return api && api->SetString &&
           api->SetString(api->context, handle, path, value);
  }

  /**
   * @brief Reads an integer, returning fallback if unavailable.
   */
  [[nodiscard]] static std::int64_t GetInt(DocumentHandle handle,
                                           const char* path,
                                           std::int64_t fallback = 0) {
    const auto* api = GetAPI();

    return api && api->GetInt
               ? api->GetInt(api->context, handle, path, fallback)
               : fallback;
  }

  [[nodiscard]] static double GetFloat(DocumentHandle handle, const char* path,
                                       double fallback = 0.0) {
    const auto* api = GetAPI();

    return api && api->GetFloat
               ? api->GetFloat(api->context, handle, path, fallback)
               : fallback;
  }

  [[nodiscard]] static bool GetBool(DocumentHandle handle, const char* path,
                                    bool fallback = false) {
    const auto* api = GetAPI();

    return api && api->GetBool
               ? api->GetBool(api->context, handle, path, fallback)
               : fallback;
  }

  /**
   * @brief Reads a string into game-owned memory.
   *
   * The first callback obtains the required buffer capacity.
   * The second copies the string across the module boundary.
   *
   * @note An empty return value can mean either an empty string
   *       or a failed read in the current API.
   */
  [[nodiscard]] static std::string GetString(DocumentHandle handle,
                                             const char* path) {
    const auto* api = GetAPI();

    if (!api || !api->GetString || !path) return {};

    const std::uint64_t required =
        api->GetString(api->context, handle, path, nullptr, 0);

    if (required == 0) return {};

    // Reject a size that cannot fit in this module's container.
    std::vector<char> buffer;

    if (required > buffer.max_size()) return {};

    buffer.resize(static_cast<std::size_t>(required));

    const std::uint64_t written =
        api->GetString(api->context, handle, path, buffer.data(), required);

    if (written != required || buffer.back() != '\0') return {};

    return std::string(buffer.data(), buffer.size() - 1);
  }

  /**
   * @brief Writes a 2D vector into a serialization document.
   *
   * The vector is stored as two nested floating-point values:
   *
   * @code
   * player:
   *   position:
   *     x: 640
   *     y: 360
   * @endcode
   *
   * This method uses the existing SetFloat() public API.
   * It does not communicate with yaml-cpp directly.
   *
   * @param document Handle to the destination document.
   * @param path Parent path for the vector.
   * @param value Vector to serialize.
   *
   * @return True if both coordinates were written successfully.
   *
   * @note The two writes are not atomic. If writing the second
   * coordinate fails, the first coordinate may already have changed.
   */
  [[nodiscard]] static bool SetVector2(DocumentHandle document,
                                       const std::string& path, Vector2 value) {
    // An empty parent path would produce invalid coordinate paths.
    if (path.empty()) {
      return false;
    }

    const std::string xPath = path + ".x";
    const std::string yPath = path + ".y";

    // Do not use short-circuit evaluation here.
    // Attempt both coordinate writes and report whether both succeeded.
    const bool xWritten = SetFloat(document, xPath.c_str(), value.x);
    const bool yWritten = SetFloat(document, yPath.c_str(), value.y);

    return xWritten && yWritten;
  }

  /**
   * @brief Reads a 2D vector from a serialization document.
   *
   * Reads the "x" and "y" fields below the specified parent path.
   *
   * Missing or invalid coordinates use their corresponding components
   * from the fallback vector.
   *
   * @param document Handle to the source document.
   * @param path Parent path for the vector.
   * @param fallback Value to use when coordinates cannot be read.
   *
   * @return The reconstructed Vector2.
   *
   * @note Each component is read independently. A document containing
   * only "x" can therefore return a combination of the stored x
   * coordinate and the fallback y coordinate.
   */
  [[nodiscard]] static Vector2 GetVector2(DocumentHandle document,
                                          const std::string& path,
                                          Vector2 fallback = {0.0f, 0.0f}) {
    if (path.empty()) {
      return fallback;
    }

    // GetFloat() returns the provided fallback when a component
    // cannot be read.
    const std::string xPath = path + ".x";
    const std::string yPath = path + ".y";
    const double x =
        GetFloat(document, xPath.c_str(), static_cast<double>(fallback.x));

    const double y =
        GetFloat(document, yPath.c_str(), static_cast<double>(fallback.y));

    return {static_cast<float>(x), static_cast<float>(y)};
  }

  /**
   * @brief Writes a 3D vector into a serialization document.
   *
   * Each component is stored under the specified parent path.
   *
   * Example:
   *
   * @code
   * camera:
   *   position:
   *     x: 10
   *     y: 20
   *     z: 30
   * @endcode
   *
   * Uses the existing SetFloat() API. No additional host
   * callbacks or yaml-cpp dependencies are required.
   *
   * @param document Handle to the destination document.
   * @param path Parent path for the vector.
   * @param value Vector to serialize.
   *
   * @return True if all three components were written.
   *
   * @note The writes are not atomic. A failed operation may
   * leave some components updated.
   */
  [[nodiscard]] static bool SetVector3(DocumentHandle document,
                                       const std::string& path, Vector3 value) {
    if (path.empty()) {
      return false;
    }

    const std::string xPath = path + ".x";
    const std::string yPath = path + ".y";
    const std::string zPath = path + ".z";

    // Attempt every write, even if an earlier write fails.
    const bool xWritten = SetFloat(document, xPath.c_str(), value.x);

    const bool yWritten = SetFloat(document, yPath.c_str(), value.y);

    const bool zWritten = SetFloat(document, zPath.c_str(), value.z);

    return xWritten && yWritten && zWritten;
  }

  /**
   * @brief Reads a 3D vector from a serialization document.
   *
   * Each coordinate is read independently from the specified
   * parent path.
   *
   * Missing or invalid coordinates use the corresponding
   * component from the fallback vector.
   *
   * @param document Handle to the source document.
   * @param path Parent path for the vector.
   * @param fallback Default value for missing components.
   *
   * @return Reconstructed Vector3.
   *
   * @note This method does not require all three coordinates
   * to exist. For strict validation, a future TryGetVector3()
   * method could report whether the complete vector exists.
   */
  [[nodiscard]] static Vector3 GetVector3(DocumentHandle document,
                                          const std::string& path,
                                          Vector3 fallback = {0.0f, 0.0f,
                                                              0.0f}) {
    if (path.empty()) {
      return fallback;
    }

    const std::string xPath = path + ".x";
    const std::string yPath = path + ".y";
    const std::string zPath = path + ".z";

    const double x =
        GetFloat(document, xPath.c_str(), static_cast<double>(fallback.x));

    const double y =
        GetFloat(document, yPath.c_str(), static_cast<double>(fallback.y));

    const double z =
        GetFloat(document, zPath.c_str(), static_cast<double>(fallback.z));

    return {static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(z)};
  }

  /**
   * @brief Saves a document through the host.
   */
  static bool Save(DocumentHandle handle, const char* path) {
    const auto* api = GetAPI();

    return api && api->Save && api->Save(api->context, handle, path);
  }

  /**
   * @brief Loads a document through the host.
   */
  [[nodiscard]] static DocumentHandle Load(const char* path) {
    const auto* api = GetAPI();

    return api && api->Load ? api->Load(api->context, path)
                            : InvalidDocumentHandle;
  }

  /**
   * @brief Creates or replaces an array in a document.
   *
   * The array is initially empty.
   *
   * @param document Destination document.
   * @param path Destination path.
   *
   * @return True if the array was created.
   */
  [[nodiscard]] static bool CreateArray(DocumentHandle document,
                                        const char* path) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->CreateArray == nullptr) {
      return false;
    }

    if (path == nullptr || *path == '\0') {
      return false;
    }

    return api->CreateArray(api->context, document, path);
  }

  /**
   * @brief Returns the number of elements in an array.
   *
   * Returns zero if the array is empty or unavailable.
   */
  [[nodiscard]] static std::uint64_t GetArraySize(DocumentHandle document,
                                                  const char* path) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->GetArraySize == nullptr) {
      return 0;
    }

    if (path == nullptr || *path == '\0') {
      return 0;
    }

    return api->GetArraySize(api->context, document, path);
  }

  /**
   * @brief Appends an empty object to an existing array.
   *
   * The array must already exist.
   *
   * @return True if an object was appended.
   */
  [[nodiscard]] static bool AppendObject(DocumentHandle document,
                                         const char* path) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->AppendObject == nullptr) {
      return false;
    }

    if (path == nullptr || *path == '\0') {
      return false;
    }

    return api->AppendObject(api->context, document, path);
  }

  /**
   * @brief Appends an integer to an existing array.
   *
   * @return False if the module is unavailable, the document
   * is invalid, or the destination is not an array.
   */
  [[nodiscard]] static bool AppendInt(DocumentHandle document, const char* path,
                                      std::int64_t value) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->AppendInt == nullptr || path == nullptr ||
        *path == '\0') {
      return false;
    }

    return api->AppendInt(api->context, document, path, value);
  }

  /**
   * @brief Appends a floating-point value to an existing array.
   */
  [[nodiscard]] static bool AppendFloat(DocumentHandle document,
                                        const char* path, double value) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->AppendFloat == nullptr || path == nullptr ||
        *path == '\0') {
      return false;
    }

    return api->AppendFloat(api->context, document, path, value);
  }

  /**
   * @brief Appends a boolean to an existing array.
   */
  [[nodiscard]] static bool AppendBool(DocumentHandle document,
                                       const char* path, bool value) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->AppendBool == nullptr || path == nullptr ||
        *path == '\0') {
      return false;
    }

    return api->AppendBool(api->context, document, path, value);
  }

  /**
   * @brief Appends a string to an existing array.
   *
   * The host copies the string before returning.
   */
  [[nodiscard]] static bool AppendString(DocumentHandle document,
                                         const char* path, const char* value) {
    const SerializationAPI* api = GetAPI();

    if (api == nullptr || api->AppendString == nullptr || path == nullptr ||
        *path == '\0' || value == nullptr) {
      return false;
    }

    return api->AppendString(api->context, document, path, value);
  }

  /**
   * @brief Determines whether a document path contains an array.
   *
   * An empty array returns true.
   *
   * Missing paths, invalid document handles, and paths referring
   * to other YAML types return false.
   *
   * @param document Source document.
   * @param path Path to inspect.
   *
   * @return True if the destination is an array.
   */
  [[nodiscard]] static bool IsArray(DocumentHandle document, const char* path) {
    const SerializationAPI* api = GetAPI();

    // Validate the module and callback before crossing
    // the host/game boundary.
    if (api == nullptr || api->IsArray == nullptr || path == nullptr ||
        *path == '\0') {
      return false;
    }

    return api->IsArray(api->context, document, path);
  }

 private:
  /**
   * @brief Retrieves the versioned serialization function table.
   *
   * Never cache this pointer globally. Resolve it through the
   * currently bound HostServices whenever it is needed.
   */
  [[nodiscard]] static const SerializationAPI* GetAPI() {
    const HostServices* host = Services::Get();

    if (!host || !host->GetModuleAPI) return nullptr;

    const void* module = host->GetModuleAPI(
        host->context, ModuleID::Serialization, SERIALIZATION_API_VERSION);

    return static_cast<const SerializationAPI*>(module);
  }
};

}  // namespace Levye