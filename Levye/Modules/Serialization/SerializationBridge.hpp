#pragma once

#include <Levye/Modules/Serialization/SerializationAPI.hpp>

namespace Levye {

class SerializationService;

/**
 * @brief Creates the public callback table for a host-owned service.
 *
 * The returned table contains no yaml-cpp objects or C++ containers.
 * Its context pointer refers to the supplied service, which must
 * outlive every call through the table.
 */
[[nodiscard]] SerializationAPI MakeSerializationAPI(
    SerializationService& service);

}  // namespace Levye