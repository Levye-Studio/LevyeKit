#include <Levye/Core/Application.hpp>
#include <Levye/Core/GameAPI.hpp>
#include <Levye/HotReload/GameModule.hpp>
#include <Levye/Project/ProjectLoader.hpp>

#include <raylib.h>

#include <iostream>
#include <string>

extern "C" const Levye::GameAPI* GetGameAPI();

int main()
{
    // levye.project is packaged at the APK asset root.
    char* manifestText = LoadFileText("levye.project");

    if (!manifestText) {
        std::cerr << "[LevyeKit] Cannot read packaged levye.project\n";
        return 1;
    }

    const auto project = Levye::ProjectLoader::Parse(
        std::string_view(manifestText)
    );

    UnloadFileText(manifestText);

    if (!project) {
        std::cerr << "[LevyeKit] Invalid packaged levye.project\n";
        return 1;
    }

    Levye::GameModule game;

    // Use the asset directory declared in levye.project.
    game.SetAssetRoot(project->assetDirectory);

    if (!game.LoadStatic(GetGameAPI())) {
        return 1;
    }

    Levye::ApplicationConfig config =
        project->CreateApplicationConfig();

    // Android uses the physical screen size rather than
    // the desktop's initial window dimensions.
    //
    // Keep the logical display and scaling configuration
    // from levye.project unchanged.

    Levye::Application app(config, game);
    app.Run();

    return 0;
}