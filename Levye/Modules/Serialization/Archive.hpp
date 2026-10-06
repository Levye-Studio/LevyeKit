#pragma once

#include <raylib.h>

#include <Levye/Modules/Serialization/SerializationAPI.hpp>

//
#include <concepts>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Levye {

namespace Detail {

template <typename Archive, typename T>
concept SerializableWith = requires(Archive& archive, T& value) {
  { Serialize(archive, value) } -> std::convertible_to<bool>;
};

}  // namespace Detail

/**
 * @brief Serializes structured C++ data into a serialization document.
 *
 * WriteArchive does not own the document. It translates named fields into
 * paths understood by the host-owned Serialization module.
 */
class WriteArchive {
 public:
  WriteArchive(DocumentHandle document, std::string path)
      : m_Document(document), m_Path(std::move(path)) {}

  /**
   * @brief Writes an integer field.
   */
  bool Field(const char* name, const int& value);

  /**
   * @brief Writes a floating-point field.
   */
  bool Field(const char* name, const float& value);

  /**
   * @brief Writes a boolean field.
   */
  bool Field(const char* name, const bool& value);

  /**
   * @brief Writes a string field.
   */
  bool Field(const char* name, const std::string& value);

  /**
   * @brief Writes a Vector2 field.
   */
  bool Field(const char* name, const Vector2& value);

  /**
   * @brief Writes a Vector3 field.
   */
  bool Field(const char* name, const Vector3& value);

  /**
   * @brief Writes a Vector4 field.
   */
  bool Field(const char* name, const Vector4& value);

  /**
   * @brief Writes a Rectangle field.
   */
  bool Field(const char* name, const Rectangle& value);

  /**
   * @brief Writes a Color field.
   */
  bool Field(const char* name, const Color& value);

  /**
   * @brief Writes a vector as a serialization array.
   *
   * Each element is serialized according to its type.
   */
  template <typename T>
  bool Field(const char* name, std::vector<T>& values) {
    const std::string path = MakePath(name);

    if (!CreateArray(path)) {
      return false;
    }

    for (T& value : values) {
      if (!AppendElement(path, value)) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Writes a nested user-defined serializable object.
   *
   * The nested object is serialized below this field's path using its
   * free Serialize() function.
   */
  template <typename T>
    requires Detail::SerializableWith<WriteArchive, T>
  bool Field(const char* name, T& value) {
    WriteArchive nested(m_Document, MakePath(name));
    return Serialize(nested, value);
  }

  template <typename T>
  bool RequiredField(const char* name, T& value) {
    return Field(name, value);
  }

  template <typename T>
  bool RequiredField(const char* name, const T& value) {
    return Field(name, value);
  }

 private:
  std::string MakePath(const char* name) const {
    if (m_Path.empty()) {
      return name;
    }

    return m_Path + "." + name;
  }

  bool WriteElement(const std::string& path, int value);
  bool WriteElement(const std::string& path, float value);
  bool WriteElement(const std::string& path, bool value);
  bool WriteElement(const std::string& path, const std::string& value);

  template <typename T>
    requires Detail::SerializableWith<WriteArchive, T>
  bool WriteElement(const std::string& path, T& value) {
    WriteArchive nested(m_Document, path);
    return Serialize(nested, value);
  }

  bool AppendElement(const std::string& path, int value);
  bool AppendElement(const std::string& path, float value);
  bool AppendElement(const std::string& path, bool value);
  bool AppendElement(const std::string& path, const std::string& value);

  template <typename T>
    requires Detail::SerializableWith<WriteArchive, T>
  bool AppendElement(const std::string& path, T& value) {
    if (!AppendObject(path)) {
      return false;
    }

    const std::uint64_t count = GetArraySize(path);

    if (count == 0) {
      return false;
    }

    const std::string elementPath =
        path + "[" + std::to_string(count - 1) + "]";

    WriteArchive nested(m_Document, elementPath);

    return Serialize(nested, value);
  }

  template <typename T>
  bool AppendElement(const std::string& path, std::vector<T>& values) {
    if (!AppendArray(path)) {
      return false;
    }

    const std::uint64_t count = GetArraySize(path);

    if (count == 0) {
      return false;
    }

    const std::string nestedPath = path + "[" + std::to_string(count - 1) + "]";

    for (T& value : values) {
      if (!AppendElement(nestedPath, value)) {
        return false;
      }
    }

    return true;
  }

  bool AppendObject(const std::string& path);

  bool AppendArray(const std::string& path);

  [[nodiscard]] std::uint64_t GetArraySize(const std::string& path) const;

  bool CreateArray(const std::string& path);

 private:
  DocumentHandle m_Document;
  std::string m_Path;
};

/**
 * @brief Deserializes structured C++ data from a serialization document.
 *
 * ReadArchive does not own the document. Existing field values are used as
 * fallbacks when a value cannot be read from the document.
 */
class ReadArchive {
 public:
  ReadArchive(DocumentHandle document, std::string path)
      : m_Document(document), m_Path(std::move(path)) {}

  /**
   * @brief Reads an integer field.
   *
   * The existing value is preserved if the field cannot be read.
   */
  bool Field(const char* name, int& value) const;

  /**
   * @brief Reads a floating-point field.
   */
  bool Field(const char* name, float& value) const;

  /**
   * @brief Reads a boolean field.
   */
  bool Field(const char* name, bool& value) const;

  /**
   * @brief Reads a string field.
   */
  bool Field(const char* name, std::string& value) const;

  /**
   * @brief Reads a Vector2 field.
   */
  bool Field(const char* name, Vector2& value) const;

  /**
   * @brief Reads a Vector3 field.
   */
  bool Field(const char* name, Vector3& value) const;

  /**
   * @brief Reads a Vector4 field.
   */
  bool Field(const char* name, Vector4& value) const;

  /**
   * @brief Reads a Rectangle field.
   */
  bool Field(const char* name, Rectangle& value) const;

  /**
   * @brief Reads a Color field.
   */
  bool Field(const char* name, Color& value) const;

  /**
   * @brief Reads a serialization array into a vector.
   *
   * Existing vector contents are replaced only after the array has
   * been completely traversed.
   */
  template <typename T>
  bool Field(const char* name, std::vector<T>& values) const {
    const std::string path = MakePath(name);

    if (!IsArray(path)) {
      return true;
    }

    const std::uint64_t count = GetArraySize(path);

    std::vector<T> loaded;
    loaded.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t index = 0; index < count; ++index) {
      T value{};

      const std::string elementPath = path + "[" + std::to_string(index) + "]";

      if (!ReadElement(elementPath, value)) {
        return false;
      }

      loaded.push_back(std::move(value));
    }

    values = std::move(loaded);

    return true;
  }

  /**
   * @brief Reads a nested user-defined serializable object.
   *
   * Existing values inside the object act as fallbacks for fields that
   * are missing from the document.
   */
  template <typename T>
    requires Detail::SerializableWith<ReadArchive, T>
  bool Field(const char* name, T& value) const {
    ReadArchive nested(m_Document, MakePath(name));
    return Serialize(nested, value);
  }

  template <typename T>
  bool RequiredField(const char* name, T& value) const {
    if (!ContainsPath(MakePath(name))) {
      return false;
    }

    return Field(name, value);
  }

 private:
  std::string MakePath(const char* name) const {
    if (m_Path.empty()) {
      return name;
    }

    return m_Path + "." + name;
  }

  bool ReadElement(const std::string& path, int& value) const;
  bool ReadElement(const std::string& path, float& value) const;
  bool ReadElement(const std::string& path, bool& value) const;
  bool ReadElement(const std::string& path, std::string& value) const;

  template <typename T>
  bool ReadElement(const std::string& path, std::vector<T>& values) const {
    if (!IsArray(path)) {
      return false;
    }

    const std::uint64_t count = GetArraySize(path);

    std::vector<T> loaded;
    loaded.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t index = 0; index < count; ++index) {
      T value{};

      const std::string elementPath = path + "[" + std::to_string(index) + "]";

      if (!ReadElement(elementPath, value)) {
        return false;
      }

      loaded.push_back(std::move(value));
    }

    values = std::move(loaded);

    return true;
  }

  template <typename T>
    requires Detail::SerializableWith<ReadArchive, T>
  bool ReadElement(const std::string& path, T& value) const {
    ReadArchive nested(m_Document, path);
    return Serialize(nested, value);
  }

  [[nodiscard]] bool IsArray(const std::string& path) const;

  [[nodiscard]] std::uint64_t GetArraySize(const std::string& path) const;

  [[nodiscard]] bool ContainsPath(const std::string& path) const;

 private:
  DocumentHandle m_Document;
  std::string m_Path;
};

}  // namespace Levye