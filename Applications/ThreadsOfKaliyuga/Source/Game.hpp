#ifndef THREADS_OF_KALIYUGA_GAME_HPP
#define THREADS_OF_KALIYUGA_GAME_HPP

#include <Angaraka/Base.hpp>

namespace Angaraka {
    class DirectX12GraphicsSystem;

    namespace Core {
        class BundleManager;
        class CachedResourceManager;
    }
}

namespace ThreadsOfKaliyuga
{
    class InputSystem;

    class Game
    {
    public:
        Game() = default;
        ~Game() = default;

        bool Initialize();
        void Run();
        void Shutdown();

    private:

    private:
        Angaraka::DirectX12GraphicsSystem* m_graphicsSystem{ nullptr };
        Angaraka::Core::CachedResourceManager* m_resourceManager{ nullptr };
        Angaraka::Core::BundleManager* m_bundleManager{ nullptr };
        InputSystem* m_inputSystem{ nullptr };

        // --- Timing ---
        LARGE_INTEGER m_perfFreq;     // Performance Counter Frequency
        LARGE_INTEGER m_lastTime;     // Time at the end of the last frame
        Angaraka::F32 m_deltaTime{ 0.0f };    // Time elapsed since last frame (in seconds)

    };
}


#endif // THREADS_OF_KALIYUGA_GAME_HPP