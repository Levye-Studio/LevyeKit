#include "ProjectConfig.hpp"

namespace Levye {

ApplicationConfig ProjectConfig::CreateApplicationConfig() const {
  return {.title = name,
          .width = windowWidth,
          .height = windowHeight,
          .targetFPS = targetFPS,
          .resizable = resizable,
          .vsync = vsync};
}

} // namespace Levye