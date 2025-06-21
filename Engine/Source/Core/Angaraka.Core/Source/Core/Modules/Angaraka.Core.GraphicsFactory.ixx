// IGraphicsResourceFactory.hpp - Abstract graphics interface
module;

#include <Angaraka/Base.hpp>

export module Angaraka.Core.GraphicsFactory;

import Angaraka.Core.Resources;

namespace Angaraka::Core {

    /**
     * @brief Abstract factory for graphics resource creation
     */
    export class IGraphicsResourceFactory {
    public:
        virtual ~IGraphicsResourceFactory() = default;

        // Graphics resource creation
        virtual std::shared_ptr<Resource> CreateTexture(const std::string& id, const ImageData& data) = 0;
        virtual std::shared_ptr<Resource> CreateMesh(const std::string& id, const void* meshData) = 0;
        virtual std::shared_ptr<Resource> CreateShader(const std::string& id, const void* shaderData) = 0;

        // Memory estimation for caching
        virtual size_t EstimateTextureMemory(const ImageData& data) const = 0;
        virtual size_t EstimateMeshMemory(const void* meshData) const = 0;
    };
}