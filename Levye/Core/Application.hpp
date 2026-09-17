#pragma once

#include <string>

namespace Levye {
struct ApplicationConfig {
  int width = 1280;
  int height = 720;

  std::string title = "Levye Game";

  int targetFPS = 60;
};

class Application {
public:
  explicit Application(const ApplicationConfig &config);
  ~Application();

  void Run();

private:
  ApplicationConfig m_Config;
};
} // namespace Levye