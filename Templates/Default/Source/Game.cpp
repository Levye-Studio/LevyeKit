#include <Levye/LevyeKit.hpp>

#include <new>

namespace
{

/**
 * @brief Persistent state owned by the generated game.
 *
 * The host owns the raw memory containing this structure, allowing the state
 * to survive compatible game-code hot reloads.
 *
 * Changing the layout of this structure while the game is running requires
 * restarting the game.
 */
struct GameState
{
    // Add persistent game data here.
};

/**
 * @brief Converts host-owned state memory to the game's concrete state type.
 *
 * @param state Pointer to the persistent state memory provided by LevyeKit.
 * @return Pointer to the game's persistent state.
 */
GameState* GetState(void* state)
{
    return static_cast<GameState*>(state);
}

/**
 * @brief Binds the LevyeKit host services to the game module.
 *
 * The host calls this function whenever the game module is loaded or
 * reloaded. Passing nullptr detaches the module from the host services.
 *
 * @param services Host services provided by LevyeKit, or nullptr when
 * detaching the game module.
 */
void BindServices(
    const Levye::HostServices* services
)
{
    if (services)
    {
        Levye::Services::Bind(services);
    }
    else
    {
        Levye::Services::Unbind();
    }
}

/**
 * @brief Constructs the game's persistent state.
 *
 * Called once when the game starts. This is not called during normal
 * hot reloads.
 *
 * @param state Host-owned memory where GameState will be constructed.
 */
void InitializeState(
    void* state
)
{
    new (state) GameState{};
}

/**
 * @brief Destroys the game's persistent state.
 *
 * The host releases the underlying memory after this callback returns.
 *
 * @param state Persistent game state.
 */
void DestroyState(
    void* state
)
{
    GetState(state)->~GameState();
}

/**
 * @brief Called once after the game module has started.
 */
void OnLoad(
    void* state
)
{
    (void)state;

    Levye::Log::Info(
        "{{PROJECT_NAME}} started."
    );
}

/**
 * @brief Called before the current game module is replaced during hot reload.
 */
void OnBeforeReload(
    void* state
)
{
    (void)state;

    Levye::Log::Info(
        "{{PROJECT_NAME}} preparing for code reload."
    );
}

/**
 * @brief Called after new game code has been successfully hot reloaded.
 */
void OnAfterReload(
    void* state
)
{
    (void)state;

    Levye::Log::Info(
        "{{PROJECT_NAME}} code reloaded."
    );
}

/**
 * @brief Updates frame-based game logic.
 *
 * @param state Persistent game state.
 * @param deltaTime Scaled time elapsed since the previous frame.
 */
void OnUpdate(
    void* state,
    float deltaTime
)
{
    (void)state;
    (void)deltaTime;
}

/**
 * @brief Updates fixed-timestep simulation logic.
 *
 * @param state Persistent game state.
 * @param fixedDeltaTime Duration of the current fixed simulation step.
 */
void OnFixedUpdate(
    void* state,
    float fixedDeltaTime
)
{
    (void)state;
    (void)fixedDeltaTime;
}

/**
 * @brief Draws the current game frame.
 */
void OnDraw(
    void* state
)
{
    (void)state;

    ClearBackground(BLACK);

    DrawText(
        "{{PROJECT_NAME}}",
        40,
        40,
        32,
        RAYWHITE
    );
}

/**
 * @brief Called once before the application shuts down.
 */
void OnShutdown(
    void* state
)
{
    (void)state;

    Levye::Log::Info(
        "{{PROJECT_NAME}} shutting down."
    );
}

} // namespace

extern "C"
const Levye::GameAPI* GetGameAPI()
{
    static const Levye::GameAPI api = {
        .version = Levye::GAME_API_VERSION,

        .BindServices = BindServices,

        .OnLoad = OnLoad,

        .OnBeforeReload = OnBeforeReload,
        .OnAfterReload = OnAfterReload,

        .OnUpdate = OnUpdate,
        .OnFixedUpdate = OnFixedUpdate,
        .OnDraw = OnDraw,
        .OnShutdown = OnShutdown,

        .InitializeState = InitializeState,
        .DestroyState = DestroyState,

        .stateSize = sizeof(GameState)
    };

    return &api;
}
