#pragma once

#include <cstdint>

namespace Levye {

/**
 * @brief Identifies a serialization document owned by the host.
 *
 * A handle is not a pointer to a YAML node. The host resolves it
 * to whichever serialization backend is enabled.
 */
using DocumentHandle = std::uint64_t;

inline constexpr DocumentHandle InvalidDocumentHandle = 0;

/**
 * @brief Supported serialization formats.
 *
 * Additional formats can be introduced without exposing their
 * implementation libraries to game modules.
 */
enum class SerializationFormat : std::uint32_t { Auto = 0, YAML = 1 };

/**
 * @brief Lightweight reference to a host-owned document.
 *
 * This type stores only a handle. It does not own a YAML object
 * or expose implementation-specific data.
 */
class Document {
 public:
  constexpr Document() = default;

  explicit constexpr Document(DocumentHandle handle) : m_Handle(handle) {}

  /**
   * @brief Returns whether this document has a nonzero handle.
   *
   * This does not guarantee that the host still owns the handle.
   */
  [[nodiscard]] constexpr bool IsValid() const {
    return m_Handle != InvalidDocumentHandle;
  }

  /**
   * @brief Returns the underlying host document handle.
   */
  [[nodiscard]] constexpr DocumentHandle Handle() const { return m_Handle; }

 private:
  DocumentHandle m_Handle = InvalidDocumentHandle;
};

}  // namespace Levye