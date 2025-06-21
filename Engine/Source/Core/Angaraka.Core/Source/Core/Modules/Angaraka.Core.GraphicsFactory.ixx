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
        std::shared_ptr<Resource> CreateTexture(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateTextureImpl(asset, context);
        }

        std::shared_ptr<Resource> CreateMesh(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateMeshImpl(asset, context);
        }

        std::shared_ptr<Resource> CreateMaterial(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateMaterialImpl(asset, context);
        }

        std::shared_ptr<Resource> CreateSound(const AssetDefinition& asset, void* context) {
            return static_cast<Derived*>(this)->CreateSoundImpl(asset, context);
        }

    protected:
        ~IGraphicsResourceFactory() = default;
    };



    // Type-erased wrapper for storage
    export class GraphicsResourceFactory {
    public:
        template<typename T>
        GraphicsResourceFactory(T&& factory) : m_factory(std::make_unique<FactoryWrapper<T>>(std::forward<T>(factory))) {}

        std::shared_ptr<Resource> Create(const AssetDefinition& asset, void* context) {
            std::filesystem::path assetPath(asset.path);
            if (assetPath.extension() == ".png" || assetPath.extension() == ".jpg" || assetPath.extension() == ".jpeg") {
                return CreateTexture(asset, context);
            } else if (assetPath.extension() == ".mesh") {
                return CreateMesh(asset, context);
            } else if (assetPath.extension() == ".material") {
                return CreateMaterial(asset, context);
            } else if (assetPath.extension() == ".wav" || assetPath.extension() == ".mp3") {
                return CreateSound(asset, context);
            }
        }

        std::shared_ptr<Resource> CreateTexture(const AssetDefinition& asset, void* context) {
            return m_factory->CreateTexture(asset, context);
        }

        std::shared_ptr<Resource> CreateMesh(const AssetDefinition& asset, void* context) {
            return m_factory->CreateMesh(asset, context);
        }

        std::shared_ptr<Resource> CreateMaterial(const AssetDefinition& asset, void* context) {
            return m_factory->CreateMaterial(asset, context);
        }

        std::shared_ptr<Resource> CreateSound(const AssetDefinition& asset, void* context) {
            return m_factory->CreateSound(asset, context);
        }

    private:
        struct IFactoryWrapper {
            virtual ~IFactoryWrapper() = default;
            virtual std::shared_ptr<Resource> CreateTexture(const AssetDefinition& asset, void* context) = 0;
            virtual std::shared_ptr<Resource> CreateMesh(const AssetDefinition& asset, void* context) = 0;
            virtual std::shared_ptr<Resource> CreateMaterial(const AssetDefinition& asset, void* context) = 0;
            virtual std::shared_ptr<Resource> CreateSound(const AssetDefinition& asset, void* context) = 0;
        };

        template<typename T>
        struct FactoryWrapper : IFactoryWrapper {
            T factory;
            FactoryWrapper(T&& f) : factory(std::forward<T>(f)) {}

            std::shared_ptr<Resource> CreateTexture(const AssetDefinition& asset, void* context) override {
                return factory.CreateTexture(asset, context);
            }
            std::shared_ptr<Resource> CreateMesh(const AssetDefinition& asset, void* context) override {
                return factory.CreateMesh(asset, context);
            }
            std::shared_ptr<Resource> CreateMaterial(const AssetDefinition& asset, void* context) override {
                return factory.CreateMaterial(asset, context);
            }
            std::shared_ptr<Resource> CreateSound(const AssetDefinition& asset, void* context) override {
                return factory.CreateSound(asset, context);
            }
        };

        std::unique_ptr<IFactoryWrapper> m_factory;
    };

    /**
     * @brief Abstract factory for graphics resource creation
     */
    class IGraphicsResourceFactory1 {
    public:
        virtual ~IGraphicsResourceFactory1() = default;

        // Graphics resource creation
        virtual std::shared_ptr<Resource> CreateTexture(const std::string& id, const ImageData& data) = 0;
        virtual std::shared_ptr<Resource> CreateMesh(const std::string& id, const void* meshData) = 0;
        virtual std::shared_ptr<Resource> CreateShader(const std::string& id, const void* shaderData) = 0;

        // Memory estimation for caching
        virtual size_t EstimateTextureMemory(const ImageData& data) const = 0;
        virtual size_t EstimateMeshMemory(const void* meshData) const = 0;
    };
}