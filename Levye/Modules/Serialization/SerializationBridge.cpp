#include "SerializationBridge.hpp"

#include <cstring>
#include <string>

#include "SerializationService.hpp"

namespace Levye {
namespace {

SerializationService* GetService(void* context) {
  return static_cast<SerializationService*>(context);
}

DocumentHandle CreateDocument(void* context, SerializationFormat format) {
  auto* service = GetService(context);

  return service ? service->Create(format) : InvalidDocumentHandle;
}

bool DestroyDocument(void* context, DocumentHandle handle) {
  auto* service = GetService(context);

  return service && service->Destroy(handle);
}

bool ContainsDocument(void* context, DocumentHandle handle) {
  auto* service = GetService(context);

  return service && service->Contains(handle);
}

bool ContainsPathDocument(void* context, DocumentHandle handle,
                          const char* path) {
  auto* service = GetService(context);

  return service && service->ContainsPath(handle, path);
}

bool WriteInt(void* context, DocumentHandle handle, const char* path,
              std::int64_t value) {
  auto* service = GetService(context);

  return service && path && service->SetInt(handle, path, value);
}

bool WriteFloat(void* context, DocumentHandle handle, const char* path,
                double value) {
  auto* service = GetService(context);

  return service && path && service->SetFloat(handle, path, value);
}

bool WriteBool(void* context, DocumentHandle handle, const char* path,
               bool value) {
  auto* service = GetService(context);

  return service && path && service->SetBool(handle, path, value);
}

bool WriteString(void* context, DocumentHandle handle, const char* path,
                 const char* value) {
  auto* service = GetService(context);

  return service && path && value && service->SetString(handle, path, value);
}

std::int64_t ReadInt(void* context, DocumentHandle handle, const char* path,
                     std::int64_t fallback) {
  auto* service = GetService(context);

  return service && path ? service->GetInt(handle, path, fallback) : fallback;
}

double ReadFloat(void* context, DocumentHandle handle, const char* path,
                 double fallback) {
  auto* service = GetService(context);

  return service && path ? service->GetFloat(handle, path, fallback) : fallback;
}

bool ReadBool(void* context, DocumentHandle handle, const char* path,
              bool fallback) {
  auto* service = GetService(context);

  return service && path ? service->GetBool(handle, path, fallback) : fallback;
}

std::uint64_t ReadString(void* context, DocumentHandle handle, const char* path,
                         char* buffer, std::uint64_t bufferSize) {
  auto* service = GetService(context);

  if (!service || !path || !service->Contains(handle)) return 0;

  const std::string value = service->GetString(handle, path);

  // Include the null terminator in the required capacity.
  const std::uint64_t required = static_cast<std::uint64_t>(value.size()) + 1;

  // A null buffer or insufficient capacity is a size query.
  // Never partially write a string.
  if (!buffer || bufferSize < required) return required;

  std::memcpy(buffer, value.c_str(), required);

  return required;
}

bool SaveDocument(void* context, DocumentHandle handle, const char* path) {
  auto* service = GetService(context);

  return service && path && service->Save(handle, path);
}

DocumentHandle LoadDocument(void* context, const char* path) {
  auto* service = GetService(context);

  return service && path ? service->Load(path) : InvalidDocumentHandle;
}

/**
 * @brief Forwards array creation to the host-owned service.
 */
bool BridgeCreateArray(void* context, DocumentHandle document,
                       const char* path) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->CreateArray(document, path);
}

/**
 * @brief Forwards an array-size query to the host-owned service.
 */
std::uint64_t BridgeGetArraySize(void* context, DocumentHandle document,
                                 const char* path) {
  if (context == nullptr || path == nullptr) {
    return 0;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->GetArraySize(document, path);
}

/**
 * @brief Forwards object appending to the host-owned service.
 */
bool BridgeAppendObject(void* context, DocumentHandle document,
                        const char* path) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendObject(document, path);
}

/**
 * @brief Forwards object appending to the host-owned service.
 */
bool BridgeAppendArray(void* context, DocumentHandle document,
                       const char* path) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendArray(document, path);
}

/**
 * @brief Forwards integer appending to the host service.
 */
bool BridgeAppendInt(void* context, DocumentHandle document, const char* path,
                     std::int64_t value) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendInt(document, path, value);
}

/**
 * @brief Forwards floating-point appending to the host service.
 */
bool BridgeAppendFloat(void* context, DocumentHandle document, const char* path,
                       double value) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendFloat(document, path, value);
}

/**
 * @brief Forwards boolean appending to the host service.
 */
bool BridgeAppendBool(void* context, DocumentHandle document, const char* path,
                      bool value) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendBool(document, path, value);
}

/**
 * @brief Forwards string appending to the host service.
 */
bool BridgeAppendString(void* context, DocumentHandle document,
                        const char* path, const char* value) {
  if (context == nullptr || path == nullptr || value == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->AppendString(document, path, value);
}

/**
 * @brief Forwards an array-type query to the host-owned service.
 *
 * The bridge validates ABI arguments before invoking the
 * internal serialization implementation.
 */
bool BridgeIsArray(void* context, DocumentHandle document, const char* path) {
  if (context == nullptr || path == nullptr) {
    return false;
  }

  auto* service = static_cast<SerializationService*>(context);

  return service->IsArray(document, path);
}

}  // namespace

SerializationAPI MakeSerializationAPI(SerializationService& service) {
  return {.context = &service,

          .Create = CreateDocument,
          .Destroy = DestroyDocument,
          .Contains = ContainsDocument,
          .ContainsPath = ContainsPathDocument,

          .SetInt = WriteInt,
          .SetFloat = WriteFloat,
          .SetBool = WriteBool,
          .SetString = WriteString,

          .GetInt = ReadInt,
          .GetFloat = ReadFloat,
          .GetBool = ReadBool,
          .GetString = ReadString,

          .Save = SaveDocument,
          .Load = LoadDocument,

          .CreateArray = &BridgeCreateArray,
          .GetArraySize = &BridgeGetArraySize,
          .AppendObject = &BridgeAppendObject,
          .AppendArray = &BridgeAppendArray,

          .AppendInt = &BridgeAppendInt,
          .AppendFloat = &BridgeAppendFloat,
          .AppendBool = &BridgeAppendBool,
          .AppendString = &BridgeAppendString,

          .IsArray = &BridgeIsArray

  };
}

}  // namespace Levye