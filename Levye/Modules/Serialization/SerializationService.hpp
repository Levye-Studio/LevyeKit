#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "Document.hpp"

namespace Levye {

/**
 * @brief Manages serialization documents owned by the host.
 *
 * Documents are identified by opaque handles so reloadable game modules
 * never receive pointers to backend-specific objects.
 */
class SerializationService {
 public:
  SerializationService();
  ~SerializationService();

  SerializationService(const SerializationService&) = delete;
  SerializationService& operator=(const SerializationService&) = delete;
  /**
   * @brief Creates an empty document using the requested format.
   *
   * @return A valid handle, or InvalidDocumentHandle on failure.
   */
  DocumentHandle Create(SerializationFormat format = SerializationFormat::YAML);

  /**
   * @brief Destroys a document and releases its resources.
   *
   * @return true if the document existed.
   */
  bool Destroy(DocumentHandle handle);

  /**
   * @brief Saves a document to a YAML file.
   *
   * Writes to a temporary file first, then replaces the destination.
   *
   * @param handle Handle of the document to save.
   * @param path Destination file path.
   * @return true if the file was saved successfully.
   */
  bool Save(DocumentHandle handle, const std::string& path);

  /**
   * @brief Loads a YAML file into a new host-owned document.
   *
   * @param path Source file path.
   * @return A new document handle, or InvalidDocumentHandle on failure.
   */
  DocumentHandle Load(const std::string& path);

  /**
   * @brief Checks whether a document is currently registered.
   */
  [[nodiscard]] bool Contains(DocumentHandle handle) const;

  /**
   * @brief Writes an integer to a document.
   *
   * @return false if the document or path is invalid.
   */
  bool SetInt(DocumentHandle handle, const std::string& path,
              std::int64_t value);

  /**
   * @brief Writes a floating-point value to a document.
   */
  bool SetFloat(DocumentHandle handle, const std::string& path, double value);

  /**
   * @brief Writes a boolean value to a document.
   */
  bool SetBool(DocumentHandle handle, const std::string& path, bool value);

  /**
   * @brief Writes a string value to a document.
   */
  bool SetString(DocumentHandle handle, const std::string& path,
                 const std::string& value);

  /**
   * @brief Reads an integer, returning the fallback if unavailable.
   */
  std::int64_t GetInt(DocumentHandle handle, const std::string& path,
                      std::int64_t fallback = 0) const;

  /**
   * @brief Reads a floating-point value.
   */
  double GetFloat(DocumentHandle handle, const std::string& path,
                  double fallback = 0.0) const;

  /**
   * @brief Reads a boolean value.
   */
  bool GetBool(DocumentHandle handle, const std::string& path,
               bool fallback = false) const;

  /**
   * @brief Reads a string value.
   */
  std::string GetString(DocumentHandle handle, const std::string& path,
                        const std::string& fallback = "") const;

  /**
   * @brief Creates an empty sequence at the specified path.
   *
   * If the path already contains a value, this operation replaces
   * that value with an empty sequence.
   *
   * @param handle Handle to the destination document.
   * @param path Destination path.
   *
   * @return True if the sequence was created.
   */
  [[nodiscard]] bool CreateArray(DocumentHandle handle, const char* path);

  /**
   * @brief Returns the number of elements in an array.
   *
   * Returns zero for an empty array, invalid handle,
   * missing path, or a path that is not an array.
   *
   * @param handle Handle to the source document.
   * @param path Path to the sequence.
   *
   * @return Number of elements.
   */
  [[nodiscard]] std::uint64_t GetArraySize(DocumentHandle handle,
                                           const char* path) const;

  /**
   * @brief Appends an empty object to an existing array.
   *
   * The array must already exist.
   *
   * @param handle Handle to the destination document.
   * @param path Path to the sequence.
   *
   * @return True if an object was appended.
   */
  [[nodiscard]] bool AppendObject(DocumentHandle handle, const char* path);

  /**
   * @brief Appends an integer to an existing YAML sequence.
   *
   * The destination must already be an array.
   *
   * @param handle Destination document.
   * @param path Path to the array.
   * @param value Integer to append.
   *
   * @return True if the integer was appended.
   */
  [[nodiscard]] bool AppendInt(DocumentHandle handle, const char* path,
                               std::int64_t value);

  /**
   * @brief Appends a floating-point value to an existing array.
   *
   * @param handle Destination document.
   * @param path Path to the array.
   * @param value Value to append.
   *
   * @return True if the value was appended.
   */
  [[nodiscard]] bool AppendFloat(DocumentHandle handle, const char* path,
                                 double value);

  /**
   * @brief Appends a boolean value to an existing array.
   *
   * @param handle Destination document.
   * @param path Path to the array.
   * @param value Value to append.
   *
   * @return True if the value was appended.
   */
  [[nodiscard]] bool AppendBool(DocumentHandle handle, const char* path,
                                bool value);

  /**
   * @brief Appends a string to an existing array.
   *
   * @param handle Destination document.
   * @param path Path to the array.
   * @param value String to append.
   *
   * @return True if the value was appended.
   */
  [[nodiscard]] bool AppendString(DocumentHandle handle, const char* path,
                                  const std::string& value);

  /**
   * @brief Checks whether a path refers to a YAML sequence.
   *
   * Returns false if:
   * - The document handle is invalid.
   * - The path is missing or malformed.
   * - The destination exists but is not a sequence.
   *
   * An empty sequence is still considered a valid array.
   *
   * @param handle Source document.
   * @param path Path to inspect.
   *
   * @return True if the destination is a YAML sequence.
   */
  [[nodiscard]] bool IsArray(DocumentHandle handle, const char* path) const;

  /**
   * @brief Releases every document owned by this service.
   */
  void Clear();

 private:
  struct DocumentData;

  std::unordered_map<DocumentHandle, std::unique_ptr<DocumentData> >
      m_Documents;

  DocumentHandle m_NextHandle = 1;
};

}  // namespace Levye