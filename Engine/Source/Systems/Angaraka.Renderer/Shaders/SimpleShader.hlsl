// Angaraka.Graphics.DX12/Shaders/SimpleShader.hlsl

// Minimal Vertex Shader
struct VS_INPUT
{
    float3 position : POSITION; // Semantic for vertex position
    float4 color    : COLOR;    // Semantic for vertex color
    float2 texCoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Clip-space position
    float4 color : COLOR; // Pass through color if you want to tint or blend
    float2 texCoord : TEXCOORD0; // Texture coordinate for the pixel shader
};


// Constant Buffer for MVP matrix
cbuffer ModelViewProjection : register(b0)
{
    float4x4 model; // World transformation matrix
    float4x4 view; // Camera transformation matrix
    float4x4 projection; // Projection transformation matrix
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    // Apply transformations in the correct order: Model -> View -> Projection
    // Note: HLSL mul(A, B) performs A * B if A is row vector, B is column vector.
    // For matrices, it's typically vector * matrix (row-major).
    // So, position * model * view * projection.
    output.position = mul(mul(mul(float4(input.position, 0.5f), model), view), projection);
    output.color = input.color; // Pass the interpolated color to the pixel shader
    output.texCoord = input.texCoord;

    return output;
}


// Declare the texture and sampler
// t0 corresponds to register(t0) in the root signature
Texture2D g_Texture : register(t0);
// s0 corresponds to register(s0) in the root signature (if dynamic)
// Or a static sampler.
SamplerState g_Sampler : register(s1); // Using s0 here for a dynamic sampler


float4 PSMain(PS_INPUT input) : SV_TARGET
{
    // Sample the texture using the interpolated texture coordinates and sampler
    float4 texColor = g_Texture.Sample(g_Sampler, input.texCoord);

    // You can combine the texture color with the vertex color, or just use the texture color
    // return texColor * input.color; // Example: tint texture with vertex color
    return texColor; // Example: just use texture color
    // return input.color; // 
}