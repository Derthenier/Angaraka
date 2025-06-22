// Engine/Source/Systems/Angaraka.Renderer/Source/Renderer/Modules/OBJ::Loader.ixx
module;

#include "Angaraka/MeshBase.hpp"
#include <sstream>

export module Angaraka.Graphics.DirectX12.ObjLoader;

namespace Angaraka::Graphics::DirectX12 {

    export struct OBJ
    {
        struct Vertex
        {
            DirectX::XMFLOAT3 position;  // Vertex position
            DirectX::XMFLOAT3 normal;    // Vertex normal
            DirectX::XMFLOAT2 texCoord;  // Texture coordinates
            DirectX::XMFLOAT4 color;     // RGBA color (optional) = defaults to white

            inline Vertex()
                : position(0.0f, 0.0f, 0.0f)
                , normal(0.0f, 1.0f, 0.0f) // Default normal pointing up
                , texCoord(0.0f, 0.0f)
                , color(1.0f, 1.0f, 1.0f, 1.0f) // Default white color
            {
            }
        };

        struct Face
        {
            struct VertexIndex
            {
                int position; // Index into positions array
                int texCoord; // Index into texture coordinates array (-1 if not specified)
                int normal;   // Index into normals array (-1 if not specified)

                VertexIndex() : position(-1), texCoord(-1), normal(-1) {}
            };

            std::vector<VertexIndex> vertices;  // Indices of vertices in the face - usually 3 or 4 for triangles/quads
            String material;                    // Material name for this face (if any)
        };

        struct Material
        {
            String name;
            DirectX::XMFLOAT3 ambient;   // Ambient color - Ka
            DirectX::XMFLOAT3 diffuse;   // Diffuse color - Kd
            DirectX::XMFLOAT3 specular;  // Specular color - Ks
            F32 shininess;               // Shininess factor - Ns
            F32 dissolve;                // Dissolve factor - d (0.0 = opaque, 1.0 = fully transparent)

            String diffuseTexture;       // map_Kd
            String normalTexture;        // map_bump or map_Kn
            String specularTexture;      // map_Ks

            inline Material()
                : ambient(0.2f, 0.2f, 0.2f)
                , diffuse(0.8f, 0.8f, 0.8f)
                , specular(1.0f, 1.0f, 1.0f)
                , shininess(32.0f)
                , dissolve(1.0f) // Fully opaque by default
            {
            }
        };

        // Raw OBJ file data before mesh processing
        struct Data
        {
            std::vector<DirectX::XMFLOAT3> positions;  // Vertex positions
            std::vector<DirectX::XMFLOAT3> normals;    // Vertex normals
            std::vector<DirectX::XMFLOAT2> texCoords;  // Texture coordinates
            std::vector<Face> faces;                   // Faces (triangles/quads)
            std::vector<Material> materials;           // Materials defined in the OBJ file
            std::unordered_map<String, size_t> materialMap; // Maps material names to indices
            String name;                               // Name of the OBJ file (for identification)

            inline void Clear()
            {
                positions.clear();
                normals.clear();
                texCoords.clear();
                faces.clear();
                materials.clear();
                materialMap.clear();
                name.clear();
            }
        };

        // File Loader
        class Loader
        {
        public:
            Loader() = default;
            ~Loader() = default;

            inline Scope<MeshData> Load(const String& filePath, VertexLayout targetLayout = VertexLayout::PNT)
            {
                AGK_INFO("OBJ::Loader: Loading OBJ file '{}'", filePath);

                // Parse OBJ file
                Data objData;
                if (!ParseFile(filePath, objData)) {
                    AGK_ERROR("OBJ::Loader: Failed to parse OBJ file '{}'", filePath);
                    return nullptr;
                }

                // Convert to MeshData
                auto meshData = std::make_unique<MeshData>();
                if (!ConvertToMeshData(objData, *meshData, targetLayout)) {
                    AGK_ERROR("OBJ::Loader: Failed to convert OBJ data to MeshData for '{}'", filePath);
                    return nullptr;
                }

                AGK_INFO("OBJ::Loader: Successfully loaded '{}' - {} vertices, {} indices",
                    filePath, meshData->vertexCount, meshData->indexCount);

                return meshData;
            }

            inline Scope<MeshData> LoadWithLayout(const String& filePath, const String& layoutStr)
            {
                VertexLayout layout = VertexLayoutFactory::StringToLayout(layoutStr);
                return Load(filePath, layout);
            }

            const String& GetLastError() const;

        private:
            String m_lastError;

            inline bool ParseFile(const String& filePath, Data& objData)
            {
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    m_lastError = "Could not open file: " + filePath;
                    return false;
                }

                objData.Clear();
                String line;
                String currentMaterial;
                int lineNumber = 0;

                while (std::getline(file, line)) {
                    lineNumber++;

                    // Skip empty lines and comments
                    if (line.empty() || line[0] == '#') continue;

                    std::istringstream iss(line);
                    String token;
                    iss >> token;

                    try {
                        if (token == "v") {
                            // Vertex position: v x y z [w]
                            DirectX::XMFLOAT3 pos;
                            iss >> pos.x >> pos.y >> pos.z;
                            objData.positions.push_back(pos);
                        }
                        else if (token == "vn") {
                            // Vertex normal: vn x y z
                            DirectX::XMFLOAT3 normal;
                            iss >> normal.x >> normal.y >> normal.z;
                            objData.normals.push_back(normal);
                        }
                        else if (token == "vt") {
                            // Texture coordinate: vt u v [w]
                            DirectX::XMFLOAT2 texCoord;
                            iss >> texCoord.x >> texCoord.y;
                            texCoord.y = 1.0f - texCoord.y; // Flip V coordinate for DirectX
                            objData.texCoords.push_back(texCoord);
                        }
                        else if (token == "f") {
                            // Face: f v1[/vt1[/vn1]] v2[/vt2[/vn2]] v3[/vt3[/vn3]] [v4[/vt4[/vn4]]]
                            if (!ParseFace(line, objData, currentMaterial)) {
                                m_lastError = "Failed to parse face at line " + std::to_string(lineNumber);
                                return false;
                            }
                        }
                        else if (token == "mtllib") {
                            // Material library: mtllib filename.mtl
                            String mtlFile;
                            iss >> mtlFile;

                            // Extract directory from OBJ path
                            String directory = GetDirectoryFromPath(filePath);
                            String mtlPath = directory + "/" + mtlFile;

                            ParseMTLFile(mtlPath, objData);
                        }
                        else if (token == "usemtl") {
                            // Use material: usemtl material_name
                            iss >> currentMaterial;
                        }
                        else if (token == "o" || token == "g") {
                            // Object/group name: o name or g name
                            iss >> objData.name;
                        }
                        // Ignore other tokens (s for smoothing groups, etc.)
                    }
                    catch (const std::exception& e) {
                        m_lastError = "Parse error at line " + std::to_string(lineNumber) + ": " + e.what();
                        return false;
                    }
                }

                // Validation
                if (objData.positions.empty()) {
                    m_lastError = "No vertex positions found in OBJ file";
                    return false;
                }

                if (objData.faces.empty()) {
                    m_lastError = "No faces found in OBJ file";
                    return false;
                }

                return true;
            }

            // Convert OBJData to MeshData with specified vertex layout
            inline bool ConvertToMeshData(const Data& objData, MeshData& meshData, VertexLayout targetLayout)
            {
                meshData.Clear();
                meshData.layout = VertexLayoutFactory::CreateLayout(targetLayout);
                meshData.name = objData.name.empty() ? "OBJ_Mesh" : objData.name;

                // Create vertex map to avoid duplicates (OBJ can have shared vertices)
                std::unordered_map<std::string, uint32_t> vertexMap;
                std::vector<Vertex> uniqueVertices;

                // Process each face
                for (const Face& face : objData.faces) {
                    if (face.vertices.size() != 3) continue; // Should be triangles by now

                    for (const auto& vertIdx : face.vertices) {
                        // Create vertex key for deduplication
                        std::string vertexKey = std::to_string(vertIdx.position) + "/" +
                            std::to_string(vertIdx.texCoord) + "/" +
                            std::to_string(vertIdx.normal);

                        uint32_t finalVertexIndex;
                        auto it = vertexMap.find(vertexKey);

                        if (it != vertexMap.end()) {
                            // Vertex already exists
                            finalVertexIndex = it->second;
                        }
                        else {
                            // Create new vertex
                            Vertex vertex;

                            // Position (required)
                            vertex.position = objData.positions[vertIdx.position];

                            // Normal (optional - calculate if missing)
                            if (vertIdx.normal >= 0) {
                                vertex.normal = objData.normals[vertIdx.normal];
                            }
                            else {
                                // Use default normal or calculate from face
                                vertex.normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
                            }

                            // Texture coordinates (optional)
                            if (vertIdx.texCoord >= 0) {
                                vertex.texCoord = objData.texCoords[vertIdx.texCoord];
                            }
                            else {
                                vertex.texCoord = DirectX::XMFLOAT2(0.0f, 0.0f);
                            }

                            // Color (default white, can be enhanced later)
                            vertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

                            finalVertexIndex = static_cast<uint32_t>(uniqueVertices.size());
                            uniqueVertices.push_back(vertex);
                            vertexMap[vertexKey] = finalVertexIndex;
                        }

                        meshData.indices.push_back(finalVertexIndex);
                    }
                }

                // Set counts
                meshData.vertexCount = static_cast<uint32_t>(uniqueVertices.size());
                meshData.indexCount = static_cast<uint32_t>(meshData.indices.size());

                // Pack vertices into interleaved buffer based on target layout
                if (!PackVertices(uniqueVertices, meshData)) {
                    m_lastError = "Failed to pack vertices into target layout";
                    return false;
                }

                // Convert materials
                ConvertMaterials(objData, meshData);

                // Calculate bounding information
                meshData.CalculateBounds();

                // Validate
                if (!meshData.IsValid()) {
                    m_lastError = "Generated mesh data is invalid";
                    return false;
                }

                return true;
            }

            // Parse MTL file for materials
            inline void ParseMTLFile(const String& mtlPath, Data& objData) {
                std::ifstream file(mtlPath);
                if (!file.is_open()) {
                    AGK_WARN("OBJ::Loader: Could not open MTL file: {}", mtlPath);
                    return;
                }

                String line;
                Material* currentMaterial = nullptr;

                while (std::getline(file, line)) {
                    if (line.empty() || line[0] == '#') continue;

                    std::istringstream iss(line);
                    String token;
                    iss >> token;

                    if (token == "newmtl") {
                        // New material
                        String name;
                        iss >> name;

                        objData.materials.emplace_back();
                        currentMaterial = &objData.materials.back();
                        currentMaterial->name = name;
                        objData.materialMap[name] = objData.materials.size() - 1;
                    }
                    else if (currentMaterial) {
                        if (token == "Ka") {
                            iss >> currentMaterial->ambient.x >> currentMaterial->ambient.y >> currentMaterial->ambient.z;
                        }
                        else if (token == "Kd") {
                            iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
                        }
                        else if (token == "Ks") {
                            iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
                        }
                        else if (token == "Ns") {
                            iss >> currentMaterial->shininess;
                        }
                        else if (token == "d" || token == "Tr") {
                            iss >> currentMaterial->dissolve;
                        }
                        else if (token == "map_Kd") {
                            iss >> currentMaterial->diffuseTexture;
                        }
                        else if (token == "map_Bump" || token == "bump") {
                            iss >> currentMaterial->normalTexture;
                        }
                        else if (token == "map_Ks") {
                            iss >> currentMaterial->specularTexture;
                        }
                    }
                }

                AGK_INFO("OBJ::Loader: Loaded {} materials from MTL file", objData.materials.size());
            }

            inline bool ParseFace(const String& line, Data& objData, String& currentMaterial)
            {
                std::istringstream iss(line);
                String token;
                iss >> token; // Skip "f"

                Face face;
                face.material = currentMaterial;

                String vertexStr;
                while (iss >> vertexStr) {
                    Face::VertexIndex vertIdx;

                    // Parse vertex index format: v[/vt[/vn]]
                    std::istringstream vertexStream(vertexStr);
                    String indexStr;

                    // Position index (required)
                    if (std::getline(vertexStream, indexStr, '/')) {
                        vertIdx.position = std::stoi(indexStr) - 1; // Convert to 0-based
                        if (vertIdx.position < 0 || vertIdx.position >= static_cast<int>(objData.positions.size())) {
                            return false;
                        }
                    }

                    // Texture coordinate index (optional)
                    if (std::getline(vertexStream, indexStr, '/') && !indexStr.empty()) {
                        vertIdx.texCoord = std::stoi(indexStr) - 1; // Convert to 0-based
                        if (vertIdx.texCoord < 0 || vertIdx.texCoord >= static_cast<int>(objData.texCoords.size())) {
                            vertIdx.texCoord = -1; // Invalid, will use default
                        }
                    }

                    // Normal index (optional)
                    if (std::getline(vertexStream, indexStr) && !indexStr.empty()) {
                        vertIdx.normal = std::stoi(indexStr) - 1; // Convert to 0-based
                        if (vertIdx.normal < 0 || vertIdx.normal >= static_cast<int>(objData.normals.size())) {
                            vertIdx.normal = -1; // Invalid, will calculate
                        }
                    }

                    face.vertices.push_back(vertIdx);
                }

                // Must have at least 3 vertices
                if (face.vertices.size() < 3) {
                    return false;
                }

                // If quad, split into two triangles
                if (face.vertices.size() == 4) {
                    // First triangle: 0, 1, 2
                    Face triangle1;
                    triangle1.material = face.material;
                    triangle1.vertices = { face.vertices[0], face.vertices[1], face.vertices[2] };
                    objData.faces.push_back(triangle1);

                    // Second triangle: 0, 2, 3
                    Face triangle2;
                    triangle2.material = face.material;
                    triangle2.vertices = { face.vertices[0], face.vertices[2], face.vertices[3] };
                    objData.faces.push_back(triangle2);
                }
                else if (face.vertices.size() == 3) {
                    // Triangle - add directly
                    objData.faces.push_back(face);
                }
                else {
                    // Polygon with >4 vertices - triangulate (simple fan triangulation)
                    for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
                        Face triangle;
                        triangle.material = face.material;
                        triangle.vertices = { face.vertices[0], face.vertices[i], face.vertices[i + 1] };
                        objData.faces.push_back(triangle);
                    }
                }

                return true;
            }

            // Pack OBJVertex data into interleaved vertex buffer
            inline bool PackVertices(const std::vector<Vertex>& vertices, MeshData& meshData) {
                const auto& layout = meshData.layout;
                meshData.vertexData.resize(meshData.vertexCount * layout.vertexSize);

                for (U32 i = 0; i < meshData.vertexCount; ++i) {
                    const Vertex& vertex = vertices[i];
                    uint8_t* vertexPtr = meshData.vertexData.data() + i * layout.vertexSize;

                    // Pack attributes based on layout
                    for (const auto& attr : layout.attributes) {
                        U8* attrPtr = vertexPtr + attr.offset;

                        switch (attr.semantic) {
                        case VertexSemantic::Position:
                            if (attr.type == VertexAttributeType::Float3) {
                                memcpy(attrPtr, &vertex.position, sizeof(DirectX::XMFLOAT3));
                            }
                            break;

                        case VertexSemantic::Normal:
                            if (attr.type == VertexAttributeType::Float3) {
                                memcpy(attrPtr, &vertex.normal, sizeof(DirectX::XMFLOAT3));
                            }
                            break;

                        case VertexSemantic::TexCoord0:
                        case VertexSemantic::TexCoord1:
                            if (attr.type == VertexAttributeType::Float2) {
                                memcpy(attrPtr, &vertex.texCoord, sizeof(DirectX::XMFLOAT2));
                            }
                            break;

                        case VertexSemantic::Color:
                            if (attr.type == VertexAttributeType::Float4) {
                                memcpy(attrPtr, &vertex.color, sizeof(DirectX::XMFLOAT4));
                            }
                            break;

                            // Bone data will be zero-initialized (for future animation support)
                        case VertexSemantic::BoneIndices:
                        case VertexSemantic::BoneWeights:
                            memset(attrPtr, 0, attr.GetSize());
                            break;

                        default:
                            // Unknown semantic - zero initialize
                            memset(attrPtr, 0, attr.GetSize());
                            break;
                        }
                    }
                }

                return true;
            }

            // Convert OBJ materials to MeshData materials
            void ConvertMaterials(const Data& objData, MeshData& meshData) {
                for (const auto& objMaterial : objData.materials) {
                    MeshData::MaterialInfo material;
                    material.name = objMaterial.name;
                    material.diffuseColor = objMaterial.diffuse;
                    material.specularPower = objMaterial.shininess;
                    material.diffuseTexture = objMaterial.diffuseTexture;
                    material.normalTexture = objMaterial.normalTexture;
                    material.specularTexture = objMaterial.specularTexture;

                    meshData.materials.push_back(material);
                }
            }

            // Helper: Extract directory path from file path
            String GetDirectoryFromPath(const String& filePath) {
                size_t lastSlash = filePath.find_last_of("/\\");
                if (lastSlash != String::npos) {
                    return filePath.substr(0, lastSlash);
                }
                return "."; // Current directory
            }
        };
    };
}