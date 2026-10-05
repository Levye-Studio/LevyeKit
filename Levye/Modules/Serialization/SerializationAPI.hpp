#pragma once

#include <Levye/Modules/Serialization/Document.hpp>
#include <cstdint>

namespace Levye {

/**
 * @brief Version of the public serialization module interface.
 *
 * Increment this when the layout or signatures of SerializationAPI
 * change. This version is independent of GAME_API_VERSION.
 */
inline constexpr std::uint32_t SERIALIZATION_API_VERSION = 4;

/**
 * @brief Public function table for the optional serialization module.
 *
 * The host owns the implementation and all document data.
 *
 * Game modules may call these functions but must never free or
 * modify the function table itself.
 *
 * All strings crossing this boundary use null-terminated C strings.
 * No std::string, YAML::Node, or STL containers cross the boundary.
 */
struct SerializationAPI {
  /**
   * @brief Opaque pointer to the host-owned serialization service.
   *
   * Pass this pointer unchanged to every callback.
   */
  void* context = nullptr;

  // -------------------------------------------------------------
  // Document lifetime
  // -------------------------------------------------------------

  /**
   * @brief Creates an empty serialization document.
   *
   * @param context Opaque serialization service context.
   * @param format Requested serialization format.
   *
   * @return A valid document handle, or InvalidDocumentHandle
   *         when creation fails.
   */
  DocumentHandle (*Create)(void* context, SerializationFormat format) = nullptr;

  /**
   * @brief Destroys a document owned by the host.
   *
   * @return True when the document existed and was destroyed.
   */
  bool (*Destroy)(void* context, DocumentHandle handle) = nullptr;

  /**
   * @brief Checks whether a document handle is currently valid.
   */
  bool (*Contains)(void* context, DocumentHandle handle) = nullptr;

  // -------------------------------------------------------------
  // Writing values
  // -------------------------------------------------------------

  bool (*SetInt)(void* context, DocumentHandle handle, const char* path,
                 std::int64_t value) = nullptr;

  bool (*SetFloat)(void* context, DocumentHandle handle, const char* path,
                   double value) = nullptr;

  bool (*SetBool)(void* context, DocumentHandle handle, const char* path,
                  bool value) = nullptr;

  bool (*SetString)(void* context, DocumentHandle handle, const char* path,
                    const char* value) = nullptr;

  // -------------------------------------------------------------
  // Reading values
  // -------------------------------------------------------------

  std::int64_t (*GetInt)(void* context, DocumentHandle handle, const char* path,
                         std::int64_t fallback) = nullptr;

  double (*GetFloat)(void* context, DocumentHandle handle, const char* path,
                     double fallback) = nullptr;

  bool (*GetBool)(void* context, DocumentHandle handle, const char* path,
                  bool fallback) = nullptr;

  /**
   * @brief Reads a string into a caller-owned buffer.
   *
   * @param buffer Destination for the null-terminated string.
   * @param bufferSize Total capacity of buffer, including '\0'.
   *
   * @return Number of bytes required, including the null
   *         terminator. Returns 0 if the value cannot be read.
   *
   * @note If bufferSize is insufficient, the callback must not
   *       write beyond the buffer. The caller can first query
   *       the required size using buffer=nullptr and
   *       bufferSize=0.
   */
  std::uint64_t (*GetString)(void* context, DocumentHandle handle,
                             const char* path, char* buffer,
                             std::uint64_t bufferSize) = nullptr;

  // -------------------------------------------------------------
  // Files
  // -------------------------------------------------------------

  bool (*Save)(void* context, DocumentHandle handle,
               const char* path) = nullptr;

  DocumentHandle (*Load)(void* context, const char* path) = nullptr;

  /**
   * @brief Creates or replaces an array at the specified path.
   *
   * @param context Host-owned serialization service context.
   * @param document Destination document.
   * @param path Dot/bracket-separated destination path.
   *
   * @return True if the array was created.
   */
  bool (*CreateArray)(void* context, DocumentHandle document,
                      const char* path) = nullptr;

  /**
   * @brief Returns the number of elements in an array.
   *
   * Returns zero for an empty array or invalid path.
   */
  std::uint64_t (*GetArraySize)(void* context, DocumentHandle document,
                                const char* path) = nullptr;

  /**
   * @brief Appends an empty mapping to an existing array.
   *
   * @return True if an object was appended.
   */
  bool (*AppendObject)(void* context, DocumentHandle document,
                       const char* path) = nullptr;

  /**
   * @brief Appends an integer to an existing array.
   */
  bool (*AppendInt)(void* context, DocumentHandle document, const char* path,
                    std::int64_t value) = nullptr;

  /**
   * @brief Appends a floating-point value to an existing array.
   */
  bool (*AppendFloat)(void* context, DocumentHandle document, const char* path,
                      double value) = nullptr;

  /**
   * @brief Appends a boolean to an existing array.
   */
  bool (*AppendBool)(void* context, DocumentHandle document, const char* path,
                     bool value) = nullptr;

  /**
   * @brief Appends a string to an existing array.
   *
   * The host copies the string into its own document.
   */
  bool (*AppendString)(void* context, DocumentHandle document, const char* path,
                       const char* value) = nullptr;

  /**
   * @brief Checks whether a document path contains a YAML sequence.
   *
   * Returns true for an existing array, including an empty one.
   * Returns false for missing paths, invalid handles, and non-arrays.
   *
   * @param context Host-owned serialization service context.
   * @param document Source document.
   * @param path Path to inspect.
   *
   * @return True if the destination is an array.
   */
  bool (*IsArray)(void* context, DocumentHandle document,
                  const char* path) = nullptr;
};

}  // namespace Levye