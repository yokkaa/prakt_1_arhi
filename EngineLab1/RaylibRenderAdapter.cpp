#include "RaylibRenderAdapter.h"
#include "raylib.h"
#include "rlgl.h"
#include <cstddef>
#include <cmath>
#include "raymath.h"

static Color ToRaylibColor(const Color4& c) {
    return Color{ c.r, c.g, c.b, c.a };
}

bool RaylibRenderAdapter::Init(int width, int height, const std::string& title) {
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    return true;
}

bool RaylibRenderAdapter::ShouldClose() const {
    return WindowShouldClose();
}

void RaylibRenderAdapter::BeginFrame() {
    BeginDrawing();
}

void RaylibRenderAdapter::Clear() {
    ClearBackground(RAYWHITE);
}

void RaylibRenderAdapter::DrawTriangle(const TransformComponent& transform, const Color4& color) {
    float size = 60.0f * transform.scale.x;

    Vector2 top{
        transform.position.x,
        transform.position.y - size
    };

    Vector2 left{
        transform.position.x - size,
        transform.position.y + size
    };

    Vector2 right{
        transform.position.x + size,
        transform.position.y + size
    };

    ::DrawTriangle(top, left, right, ToRaylibColor(color));
}

void RaylibRenderAdapter::DrawRectangle(const TransformComponent& transform, const Color4& color) {
    float width = 120.0f * transform.scale.x;
    float height = 80.0f * transform.scale.y;

    ::DrawRectangle(
        static_cast<int>(transform.position.x - width / 2.0f),
        static_cast<int>(transform.position.y - height / 2.0f),
        static_cast<int>(width),
        static_cast<int>(height),
        ToRaylibColor(color)
    );
}

GPUTexture RaylibRenderAdapter::CreateTexture(const TextureData& data) {
    GPUTexture gpuTexture;

    if (data.Pixels.empty() || data.Width <= 0 || data.Height <= 0) {
        return gpuTexture;
    }

    int format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    if (data.Channels == 1) {
        format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    }
    else if (data.Channels == 3) {
        format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    }
    else if (data.Channels == 4) {
        format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    }

    gpuTexture.ID = rlLoadTexture(
        data.Pixels.data(),
        data.Width,
        data.Height,
        format,
        1
    );

    gpuTexture.Width = data.Width;
    gpuTexture.Height = data.Height;
    gpuTexture.Channels = data.Channels;

    return gpuTexture;
}

GPUShaderProgram RaylibRenderAdapter::CreateShaderProgram(
    const std::string& vertexSource,
    const std::string& fragmentSource
) {
    GPUShaderProgram program;

    if (vertexSource.empty() || fragmentSource.empty()) {
        return program;
    }

    program.ID = rlLoadShaderCode(
        vertexSource.c_str(),
        fragmentSource.c_str()
    );

    if (program.ID == 0) {
        TraceLog(LOG_ERROR, "Failed to compile shader program");
    }
    else {
        TraceLog(LOG_INFO, "Shader program compiled successfully. ID: %u", program.ID);
    }

    return program;
}

GPUMesh RaylibRenderAdapter::UploadMesh(const MeshData& data) {
    GPUMesh gpuMesh;

    if (data.Vertices.empty() || data.Indices.empty()) {
        return gpuMesh;
    }

    if (data.Vertices.size() > 65535) {
        TraceLog(LOG_ERROR, "Mesh has too many vertices for unsigned short indices");
        return gpuMesh;
    }

    std::vector<unsigned short> indices16;
    indices16.reserve(data.Indices.size());

    for (unsigned int index : data.Indices) {
        indices16.push_back(static_cast<unsigned short>(index));
    }

    gpuMesh.IndexCount = indices16.size();

    gpuMesh.VAO = rlLoadVertexArray();
    rlEnableVertexArray(gpuMesh.VAO);

    gpuMesh.VBO = rlLoadVertexBuffer(
        data.Vertices.data(),
        static_cast<int>(data.Vertices.size() * sizeof(Vertex)),
        false
    );

    gpuMesh.EBO = rlLoadVertexBufferElement(
        indices16.data(),
        static_cast<int>(indices16.size() * sizeof(unsigned short)),
        false
    );

    rlSetVertexAttribute(
        0,
        3,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        (int)offsetof(Vertex, Position)
    );
    rlEnableVertexAttribute(0);

    rlSetVertexAttribute(
        1,
        3,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        (int)offsetof(Vertex, Normal)
    );
    rlEnableVertexAttribute(1);

    rlSetVertexAttribute(
        2,
        2,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        (int)offsetof(Vertex, TexCoords)
    );
    rlEnableVertexAttribute(2);

    rlDisableVertexArray();

    return gpuMesh;
}

void RaylibRenderAdapter::DrawMesh(
    const GPUMesh& mesh,
    const GPUTexture& texture,
    const GPUShaderProgram& shader,
    const TransformComponent& transform
) {
    if (mesh.VAO == 0 || mesh.IndexCount == 0 || shader.ID == 0) {
        return;
    }

    rlEnableShader(shader.ID);

    if (texture.ID != 0) {
        rlActiveTextureSlot(0);
        rlEnableTexture(texture.ID);

        int textureLocation = rlGetLocationUniform(shader.ID, "texture0");
        int textureSlot = 0;
        rlSetUniform(textureLocation, &textureSlot, RL_SHADER_UNIFORM_INT, 1);
    }

    Matrix matScale = MatrixScale(
        transform.scale.x,
        transform.scale.y,
        transform.scale.z
    );

    Matrix matRotX = MatrixRotateX(transform.rotation.x * DEG2RAD);
    Matrix matRotY = MatrixRotateY(transform.rotation.y * DEG2RAD);
    Matrix matRotZ = MatrixRotateZ(transform.rotation.z * DEG2RAD);

    Matrix matTranslation = MatrixTranslate(
        transform.position.x,
        transform.position.y,
        transform.position.z
    );

    Matrix matRotation = MatrixMultiply(
        MatrixMultiply(matRotX, matRotY),
        matRotZ
    );

    Matrix matModel = MatrixMultiply(
        MatrixMultiply(matScale, matRotation),
        matTranslation
    );

    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();

    Matrix matModelView = MatrixMultiply(matModel, matView);
    Matrix matMVP = MatrixMultiply(matModelView, matProjection);

    int mvpLocation = rlGetLocationUniform(shader.ID, "mvp");
    rlSetUniformMatrix(mvpLocation, matMVP);

    rlEnableVertexArray(mesh.VAO);

    rlDrawVertexArrayElements(
        0,
        static_cast<int>(mesh.IndexCount),
        nullptr
    );

    rlDisableVertexArray();

    if (texture.ID != 0) {
        rlDisableTexture();
    }

    rlEnableShader(rlGetShaderIdDefault());
}

void RaylibRenderAdapter::DeleteShaderProgram(const GPUShaderProgram& shader) {
    if (shader.ID != 0) {
        rlUnloadShaderProgram(shader.ID);
    }
}

void RaylibRenderAdapter::Begin3D() {
    Camera3D camera = { 0 };

    camera.position = Vector3{ 0.0f, 2.0f, 6.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camera);
}

void RaylibRenderAdapter::End3D() {
    EndMode3D();
}

void RaylibRenderAdapter::DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) {
    DrawText(text.c_str(), x, y, fontSize, BLACK);
}

void RaylibRenderAdapter::EndFrame() {
    EndDrawing();
}

void RaylibRenderAdapter::Shutdown() {
    CloseWindow();
}

