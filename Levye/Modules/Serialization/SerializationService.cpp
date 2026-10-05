#include "SerializationService.hpp"

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Levye {

/**
 * @brief Internal representation of a serialization document.
 *
 * This structure is defined only in the implementation file.
 */
struct SerializationService::DocumentData {
  SerializationFormat format = SerializationFormat::YAML;
  YAML::Node root{YAML::NodeType::Map};
};

namespace {

/**
 * @brief Represents one component of a serialization path.
 *
 * A path component can be either:
 * - A mapping key, such as "organisms".
 * - A sequence index, such as [0].
 */
struct PathToken {
  enum class Type { Key, Index };

  Type type = Type::Key;

  std::string key;
  std::size_t index = 0;
};

/**
 * @brief Returns true if every remaining path token is a map key.
 *
 * A null node may be replaced by a nested map only when no
 * array index needs to be created implicitly.
 */
bool RemainingTokensAreKeys(const std::vector<PathToken>& tokens,
                            std::size_t start) {
  for (std::size_t i = start; i < tokens.size(); ++i) {
    if (tokens[i].type != PathToken::Type::Key) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Parses a serialization path into mapping keys and sequence indices.
 *
 * Examples:
 *
 * "player.position.x"
 *     -> Key(player), Key(position), Key(x)
 *
 * "organisms[0].position.x"
 *     -> Key(organisms), Index(0), Key(position), Key(x)
 *
 * "world.regions[2].organisms[5].energy"
 *     -> Key(world), Key(regions), Index(2),
 *        Key(organisms), Index(5), Key(energy)
 *
 * Invalid paths return false.
 *
 * @param path Path to parse.
 * @param tokens Receives the parsed path components.
 *
 * @return True if the entire path is valid.
 */
bool ParsePath(std::string_view path, std::vector<PathToken>& tokens) {
  tokens.clear();

  if (path.empty()) {
    return false;
  }

  std::size_t position = 0;

  while (position < path.size()) {
    // -----------------------------------------------------
    // 1. Parse a mapping key
    // -----------------------------------------------------

    const std::size_t keyStart = position;

    while (position < path.size() && path[position] != '.' &&
           path[position] != '[' && path[position] != ']') {
      ++position;
    }

    // A key must contain at least one character.
    if (position == keyStart) {
      return false;
    }

    PathToken keyToken;
    keyToken.type = PathToken::Type::Key;
    keyToken.key = std::string(path.substr(keyStart, position - keyStart));

    tokens.push_back(std::move(keyToken));

    // -----------------------------------------------------
    // 2. Parse zero or more array indices
    // -----------------------------------------------------

    while (position < path.size() && path[position] == '[') {
      ++position;

      const std::size_t indexStart = position;

      while (position < path.size() && path[position] >= '0' &&
             path[position] <= '9') {
        ++position;
      }

      // Empty indices, negative indices, and nonnumeric
      // indices are not supported.
      if (position == indexStart) {
        return false;
      }

      // Every index must end with ']'.
      if (position >= path.size() || path[position] != ']') {
        return false;
      }

      const std::string_view indexText =
          path.substr(indexStart, position - indexStart);

      std::size_t index = 0;

      const auto [ptr, error] = std::from_chars(
          indexText.data(), indexText.data() + indexText.size(), index);

      if (error != std::errc{} || ptr != indexText.data() + indexText.size()) {
        return false;
      }

      PathToken indexToken;
      indexToken.type = PathToken::Type::Index;
      indexToken.index = index;

      tokens.push_back(std::move(indexToken));

      ++position;  // Skip ']'.
    }

    // -----------------------------------------------------
    // 3. Parse the separator
    // -----------------------------------------------------

    if (position == path.size()) {
      return true;
    }

    // After a key or index, only a dot may follow.
    if (path[position] != '.') {
      return false;
    }

    ++position;  // Skip '.'.

    // Reject a trailing dot.
    if (position == path.size()) {
      return false;
    }
  }

  return !tokens.empty();
}

/**
 * @brief Resolves a YAML path without modifying the document.
 *
 * Supports mapping keys and sequence indices.
 *
 * Missing keys, invalid indices, and type mismatches return
 * an undefined YAML node.
 */
YAML::Node ResolveReadPath(const YAML::Node& root, const std::string& path) {
  std::vector<PathToken> tokens;

  if (!ParsePath(path, tokens)) {
    return YAML::Node(YAML::NodeType::Undefined);
  }

  YAML::Node current;
  current.reset(root);

  try {
    for (const PathToken& token : tokens) {
      YAML::Node child;

      // -------------------------------------------------
      // Mapping key
      // -------------------------------------------------

      if (token.type == PathToken::Type::Key) {
        if (!current.IsMap()) {
          return YAML::Node(YAML::NodeType::Undefined);
        }

        // Const lookup prevents missing map keys from
        // being inserted into the document.
        const YAML::Node& readOnly = current;

        child.reset(readOnly[token.key]);
      }

      // -------------------------------------------------
      // Sequence index
      // -------------------------------------------------

      else {
        if (!current.IsSequence()) {
          return YAML::Node(YAML::NodeType::Undefined);
        }

        // Never allow an out-of-range read to extend
        // the sequence.
        if (token.index >= current.size()) {
          return YAML::Node(YAML::NodeType::Undefined);
        }

        const YAML::Node& readOnly = current;

        child.reset(readOnly[token.index]);
      }

      if (!child) {
        return YAML::Node(YAML::NodeType::Undefined);
      }

      // Rebind the local node handle without overwriting
      // the previously visited node.
      current.reset(child);
    }

    return current;

  } catch (const YAML::Exception&) {
    return YAML::Node(YAML::NodeType::Undefined);
  }
}

/**
 * @brief Resolves a validated YAML path for writing.
 *
 * Creates missing mapping nodes when needed.
 *
 * This function assumes ValidateWritePath() has already
 * accepted the path.
 *
 * Existing sequence indices must be within bounds.
 * Missing sequence elements are never created.
 *
 * @param root Root of the YAML document.
 * @param tokens Already parsed and validated destination path.
 *
 * @return Writable destination node, or an undefined node
 * if traversal fails.
 */
YAML::Node ResolveWritePath(YAML::Node root,
                            const std::vector<PathToken>& tokens) {
  if (tokens.empty()) {
    return YAML::Node(YAML::NodeType::Undefined);
  }

  try {
    YAML::Node current;
    current.reset(root);

    for (std::size_t i = 0; i < tokens.size(); ++i) {
      const PathToken& token = tokens[i];

      const bool isLast = (i + 1 == tokens.size());

      // An existing null node may become a map when
      // the next operation accesses a mapping key.
      if (current.IsNull()) {
        if (token.type != PathToken::Type::Key) {
          return YAML::Node(YAML::NodeType::Undefined);
        }

        current = YAML::Node(YAML::NodeType::Map);
      }

      if (token.type == PathToken::Type::Key) {
        if (!current.IsMap()) {
          return YAML::Node(YAML::NodeType::Undefined);
        }

        YAML::Node child = current[token.key];

        if (!child.IsDefined()) {
          // Create a missing destination or intermediate node.
          current[token.key] =
              YAML::Node(isLast ? YAML::NodeType::Null : YAML::NodeType::Map);

          child.reset(current[token.key]);
        } else if (child.IsNull() && !isLast) {
          // An existing null intermediate node becomes a map.
          current[token.key] = YAML::Node(YAML::NodeType::Map);

          child.reset(current[token.key]);
        }

        current.reset(child);
        continue;
      }

      // Sequence indices must already exist.
      if (!current.IsSequence()) {
        return YAML::Node(YAML::NodeType::Undefined);
      }

      if (token.index >= current.size()) {
        return YAML::Node(YAML::NodeType::Undefined);
      }

      YAML::Node child = current[token.index];

      // A null element is allowed to remain null when
      // it's the final destination. Otherwise, the next
      // loop iteration will turn it into a map if valid.
      current.reset(child);
    }

    return current;

  } catch (const YAML::Exception&) {
    return YAML::Node(YAML::NodeType::Undefined);
  }
}

/**
 * @brief Validates a YAML write path without modifying the tree.
 *
 * Missing map keys are allowed when the remaining path can
 * be created entirely from maps.
 *
 * Array indices are only valid when the destination sequence
 * and the requested element already exist.
 *
 * This function does not create nodes or change values.
 *
 * @param root Root of the YAML document.
 * @param tokens Parsed path to validate.
 *
 * @return True if the path can be written safely.
 */
bool ValidateWritePath(const YAML::Node& root,
                       const std::vector<PathToken>& tokens) {
  if (tokens.empty()) {
    return false;
  }

  try {
    YAML::Node current;
    current.reset(root);

    for (std::size_t i = 0; i < tokens.size(); ++i) {
      const PathToken& token = tokens[i];

      // -------------------------------------------------
      // Map key
      // -------------------------------------------------
      if (current.IsNull()) {
        return RemainingTokensAreKeys(tokens, i);
      }

      if (token.type == PathToken::Type::Key) {
        if (!current.IsMap()) {
          return false;
        }

        // Use const lookup to avoid creating a key.
        const YAML::Node& readOnly = current;

        YAML::Node child = readOnly[token.key];

        if (!child.IsDefined() || child.IsNull()) {
          // Missing/null nodes can become maps, but never implicit arrays.
          return RemainingTokensAreKeys(tokens, i + 1);
        }

        // A scalar or sequence is valid if this is
        // the final destination: the writer may
        // replace the value.
        if (i + 1 == tokens.size()) {
          return true;
        }

        current.reset(child);
        continue;
      }

      // -------------------------------------------------
      // Array index
      // -------------------------------------------------

      if (token.type == PathToken::Type::Index) {
        if (!current.IsSequence()) {
          return false;
        }

        if (token.index >= current.size()) {
          return false;
        }

        const YAML::Node& readOnly = current;

        YAML::Node child = readOnly[token.index];

        if (!child.IsDefined()) {
          return false;
        }

        // An existing array element may be replaced.
        if (i + 1 == tokens.size()) {
          return true;
        }

        current.reset(child);
        continue;
      }
    }

    return true;

  } catch (const YAML::Exception&) {
    return false;
  }
}

/**
 * @brief Writes a value to a YAML document.
 *
 * Requires a validated token sequence. Intermediate-node creation is
 * delegated to ResolveWritePath(); assignment replaces the destination value.
 *
 * @tparam T Value type.
 * @param root Root node of the document.
 * @param tokens Already parsed and validated destination path.
 * @param value Value to store.
 *
 * @return True if the value was written.
 */
template <typename T>
bool WriteValue(YAML::Node root, const std::vector<PathToken>& tokens,
                const T& value) {
  try {
    YAML::Node destination = ResolveWritePath(root, tokens);

    if (!destination.IsDefined()) {
      return false;
    }

    // Assignment intentionally replaces the value of
    // the destination node.
    destination = value;

    return true;

  } catch (const YAML::Exception&) {
    return false;
  }
}

/**
 * @brief Parses and validates once before allowing any mutation of the tree.
 *
 * Reuse the exact same tokens for validation and writing, including arrays.
 * Document-handle validation remains the service's responsibility.
 */
template <typename T>
bool SetValue(YAML::Node root, std::string_view path, const T& value) {
  std::vector<PathToken> tokens;
  if (!ParsePath(path, tokens) || !ValidateWritePath(root, tokens)) {
    return false;
  }
  return WriteValue(root, tokens, value);
}

template <typename T>
T ReadValue(const YAML::Node& root, const std::string& path,
            const T& fallback) {
  try {
    YAML::Node node = ResolveReadPath(root, path);

    if (!node || node.IsNull()) return fallback;

    return node.as<T>();
  } catch (const YAML::Exception&) {
    return fallback;
  }
}

/**
 * @brief Appends a value to an existing YAML sequence.
 *
 * This helper operates only on the YAML tree. Document-handle
 * validation remains the responsibility of SerializationService.
 *
 * @tparam T Type of the value to append.
 * @param root Root node of the document.
 * @param path Path to the destination sequence.
 * @param value Value to append.
 *
 * @return True if the value was appended.
 */
template <typename T>
bool AppendValue(const YAML::Node& root, const std::string& path,
                 const T& value) {
  if (path.empty()) {
    return false;
  }

  try {
    YAML::Node array = ResolveReadPath(root, path);

    // The destination must already exist and be a sequence.
    if (!array || !array.IsSequence()) {
      return false;
    }

    array.push_back(value);

    return true;

  } catch (const YAML::Exception&) {
    return false;
  }
}

}  // namespace

SerializationService::SerializationService() = default;
SerializationService::~SerializationService() = default;

DocumentHandle SerializationService::Create(SerializationFormat format) {
  // YAML is the only supported backend for now.
  if (format != SerializationFormat::YAML &&
      format != SerializationFormat::Auto) {
    return InvalidDocumentHandle;
  }

  const DocumentHandle handle = m_NextHandle++;

  auto document = std::make_unique<DocumentData>();

  document->format = SerializationFormat::YAML;

  m_Documents.emplace(handle, std::move(document));

  return handle;
}

bool SerializationService::Destroy(DocumentHandle handle) {
  return m_Documents.erase(handle) != 0;
}

bool SerializationService::Save(DocumentHandle handle,
                                const std::string& path) {
  namespace fs = std::filesystem;

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end() || path.empty()) return false;

  const fs::path destination(path);
  fs::path temporary = destination;

  temporary += ".tmp";

  try {
    const fs::path directory = destination.parent_path();

    if (!directory.empty()) fs::create_directories(directory);

    // Serialize the document in memory first.
    YAML::Emitter emitter;
    emitter << it->second->root;

    if (!emitter.good()) return false;

    // Write to a temporary file in the same directory.
    {
      std::ofstream file(temporary, std::ios::binary | std::ios::trunc);

      if (!file) return false;

      file << emitter.c_str() << '\n';

      file.flush();

      if (!file) return false;

      file.close();

      if (!file) return false;
    }

    // Replace the destination only after writing succeeds.
    fs::rename(temporary, destination);

    return true;

  } catch (const YAML::Exception&) {
    // Invalid YAML data or emitter failure.
  } catch (const fs::filesystem_error&) {
    // Directory creation or file replacement failed.
  } catch (const std::ios_base::failure&) {
    // File I/O failed.
  }

  // Remove an incomplete temporary file if one exists.
  std::error_code error;
  fs::remove(temporary, error);

  return false;
}

DocumentHandle SerializationService::Load(const std::string& path) {
  namespace fs = std::filesystem;

  if (path.empty()) return InvalidDocumentHandle;

  try {
    if (!fs::is_regular_file(path)) return InvalidDocumentHandle;

    YAML::Node root = YAML::LoadFile(path);

    // Our current path-based API expects a mapping at the root.
    if (!root.IsMap()) return InvalidDocumentHandle;

    const DocumentHandle handle = Create(SerializationFormat::YAML);

    if (handle == InvalidDocumentHandle) return InvalidDocumentHandle;

    // Transfer the loaded YAML tree into the registered document.
    m_Documents.at(handle)->root.reset(root);

    return handle;

  } catch (const YAML::Exception&) {
    return InvalidDocumentHandle;
  } catch (const fs::filesystem_error&) {
    return InvalidDocumentHandle;
  }
}

bool SerializationService::Contains(DocumentHandle handle) const {
  return m_Documents.contains(handle);
}

bool SerializationService::SetInt(DocumentHandle handle,
                                  const std::string& path, std::int64_t value) {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return SetValue(it->second->root, path, value);
}

bool SerializationService::SetFloat(DocumentHandle handle,
                                    const std::string& path, double value) {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return SetValue(it->second->root, path, value);
}

bool SerializationService::SetBool(DocumentHandle handle,
                                   const std::string& path, bool value) {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return SetValue(it->second->root, path, value);
}

bool SerializationService::SetString(DocumentHandle handle,
                                     const std::string& path,
                                     const std::string& value) {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return SetValue(it->second->root, path, value);
}

/**
 * @brief Creates or replaces a YAML sequence at the given path.
 */
bool SerializationService::CreateArray(DocumentHandle handle,
                                       const char* path) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return SetValue(it->second->root, path, YAML::Node(YAML::NodeType::Sequence));
}

/**
 * @brief Returns the number of elements in an existing sequence.
 */
std::uint64_t SerializationService::GetArraySize(DocumentHandle handle,
                                                 const char* path) const {
  if (path == nullptr || *path == '\0') {
    return 0;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return 0;
  }

  try {
    YAML::Node array = ResolveReadPath(it->second->root, path);

    if (!array || !array.IsSequence()) {
      return 0;
    }

    return static_cast<std::uint64_t>(array.size());

  } catch (const YAML::Exception&) {
    return 0;
  }
}

bool SerializationService::IsArray(DocumentHandle handle,
                                   const char* path) const {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  try {
    YAML::Node node = ResolveReadPath(it->second->root, path);

    return node.IsDefined() && node.IsSequence();

  } catch (const YAML::Exception&) {
    return false;
  }
}

/**
 * @brief Appends an empty mapping to an existing YAML sequence.
 */
bool SerializationService::AppendObject(DocumentHandle handle,
                                        const char* path) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return AppendValue(it->second->root, path, YAML::Node(YAML::NodeType::Map));
}
/**
 * @brief Appends an integer to an existing sequence.
 *
 * Uses the read-only path resolver to locate the array.
 * The returned YAML::Node still refers to the underlying
 * document, so push_back() modifies the actual sequence.
 */
bool SerializationService::AppendInt(DocumentHandle handle, const char* path,
                                     std::int64_t value) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return AppendValue(it->second->root, path, value);
}

/**
 * @brief Appends a double-precision floating-point value.
 */
bool SerializationService::AppendFloat(DocumentHandle handle, const char* path,
                                       double value) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return AppendValue(it->second->root, path, value);
}

/**
 * @brief Appends a boolean value.
 */
bool SerializationService::AppendBool(DocumentHandle handle, const char* path,
                                      bool value) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return AppendValue(it->second->root, path, value);
}

/**
 * @brief Appends a string value.
 */
bool SerializationService::AppendString(DocumentHandle handle, const char* path,
                                        const std::string& value) {
  if (path == nullptr || *path == '\0') {
    return false;
  }

  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) {
    return false;
  }

  return AppendValue(it->second->root, path, value);
}

std::int64_t SerializationService::GetInt(DocumentHandle handle,
                                          const std::string& path,
                                          std::int64_t fallback) const {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) return fallback;

  return ReadValue(it->second->root, path, fallback);
}

double SerializationService::GetFloat(DocumentHandle handle,
                                      const std::string& path,
                                      double fallback) const {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) return fallback;

  return ReadValue(it->second->root, path, fallback);
}

bool SerializationService::GetBool(DocumentHandle handle,
                                   const std::string& path,
                                   bool fallback) const {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) return fallback;

  return ReadValue(it->second->root, path, fallback);
}

std::string SerializationService::GetString(DocumentHandle handle,
                                            const std::string& path,
                                            const std::string& fallback) const {
  const auto it = m_Documents.find(handle);

  if (it == m_Documents.end()) return fallback;

  return ReadValue(it->second->root, path, fallback);
}

void SerializationService::Clear() { m_Documents.clear(); }

}  // namespace Levye