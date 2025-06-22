// IGraphicsResourceFactory.hpp - Abstract graphics interface
module;

#include <Angaraka/Base.hpp>
#include <Angaraka/AssetBundleConfig.hpp>

export module Angaraka.Core.GraphicsFactory;

import Angaraka.Core.Resources;
import <filesystem>;

namespace Angaraka::Core {

    // CRTP base interface
    export template<typename Derived>
    class IGraphicsResourceFactory {
    public:
        Reference<Resource> CreateTexture(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateTextureImpl(asset, context);
        }

        Reference<Resource> CreateMesh(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateMeshImpl(asset, context);
        }

        Reference<Resource> CreateMaterial(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateMaterialImpl(asset, context);
        }

        Reference<Resource> CreateSound(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateSoundImpl(asset, context);
        }

    protected:
        ~IGraphicsResourceFactory() = default;
    };



    // Type-erased wrapper for storage
    export class GraphicsResourceFactory {
    public:
        template<typename T>
        GraphicsResourceFactory(T&& factory) : m_factory(CreateScope<FactoryWrapper<T>>(std::forward<T>(factory))) {}

        Reference<Resource> Create(const AssetDefinition& asset, void* context) {
            std::filesystem::path assetPath(asset.path);
            if (assetPath.extension() == ".png" || assetPath.extension() == ".jpg" || assetPath.extension() == ".jpeg") {
                return CreateTexture(asset, context);
            } else if (assetPath.extension() == ".obj" || assetPath.extension() == ".fbx" || assetPath.extension() == ".glb" || assetPath.extension() == ".gltf") {
                return CreateMesh(asset, context);
            } else if (assetPath.extension() == ".mat") {
                return CreateMaterial(asset, context);
            } else if (assetPath.extension() == ".wav" || assetPath.extension() == ".mp3" || assetPath.extension() == ".ogg") {
                return CreateSound(asset, context);
            }

            return nullptr;
        }

        Reference<Resource> CreateTexture(const AssetDefinition& asset, void* context) {
            return m_factory->CreateTexture(asset, context);
        }

        Reference<Resource> CreateMesh(const AssetDefinition& asset, void* context) {
            return m_factory->CreateMesh(asset, context);
        }

        Reference<Resource> CreateMaterial(const AssetDefinition& asset, void* context) {
            return m_factory->CreateMaterial(asset, context);
        }

        Reference<Resource> CreateSound(const AssetDefinition& asset, void* context) {
            return m_factory->CreateSound(asset, context);
        }

    private:
        struct IFactoryWrapper {
            virtual ~IFactoryWrapper() = default;
            virtual Reference<Resource> CreateTexture(const AssetDefinition& asset, void* context) = 0;
            virtual Reference<Resource> CreateMesh(const AssetDefinition& asset, void* context) = 0;
            virtual Reference<Resource> CreateMaterial(const AssetDefinition& asset, void* context) = 0;
            virtual Reference<Resource> CreateSound(const AssetDefinition& asset, void* context) = 0;
        };

        template<typename T>
        struct FactoryWrapper : IFactoryWrapper {
            T factory;
            FactoryWrapper(T&& f) : factory(std::forward<T>(f)) {}

            Reference<Resource> CreateTexture(const AssetDefinition& asset, void* context) override {
                return factory.CreateTexture(asset, context);
            }
            Reference<Resource> CreateMesh(const AssetDefinition& asset, void* context) override {
                return factory.CreateMesh(asset, context);
            }
            Reference<Resource> CreateMaterial(const AssetDefinition& asset, void* context) override {
                return factory.CreateMaterial(asset, context);
            }
            Reference<Resource> CreateSound(const AssetDefinition& asset, void* context) override {
                return factory.CreateSound(asset, context);
            }
        };

        Scope<IFactoryWrapper> m_factory;
    };
}