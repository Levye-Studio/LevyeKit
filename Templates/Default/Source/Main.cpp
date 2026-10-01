#include <Levye/Core/Application.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Platform/ExecutablePath.hpp>
#include <Levye/Project/ProjectLoader.hpp>
#include <Levye/Platform/SharedLibrary.hpp>

#include <filesystem>
#include <iostream>

int main()
{
    const std::filesystem::path executableDirectory =
        Levye::ExecutablePath::GetDirectory();

    const std::filesystem::path targetDirectory =
        executableDirectory.parent_path();

    const std::filesystem::path projectDirectory =
        targetDirectory
            .parent_path()
            .parent_path();

    const std::filesystem::path projectFile =
        projectDirectory /
        "levye.project";

    const auto project =
        Levye::ProjectLoader::Load(
            projectFile
        );

    if (!project)
    {
        std::cerr
            << "[Levye] Failed to load project.\n";

        return 1;
    }

    const std::filesystem::path assetRoot =
        projectDirectory /
        project->assetDirectory;

    const std::filesystem::path gameModulePath =
        targetDirectory /
        "lib" /
        Levye::SharedLibrary::MakeFilename("Game");

    Levye::GameModule gameModule;

    gameModule.SetAssetRoot(
        assetRoot.string()
    );

    if (!gameModule.Load(
            gameModulePath.string()))
    {
        return 1;
    }

    const Levye::ApplicationConfig applicationConfig =
        project->CreateApplicationConfig();

    Levye::Application application{
        applicationConfig,
        gameModule
    };

    application.Run();

    return 0;
}