#include "RaylibRenderAdapter.h"
#include "RenderPreparation.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include <cstddef>
#include <cmath>
#include <vector>

static Color ToRaylibColor(const Color4& color) {
    return Color{
        color.r,
        color.g,
        color.b,
        color.a
    };
}

bool RaylibRenderAdapter::Init(
    int width,
    int height,
    const std::string& title
) {
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);

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

void RaylibRenderAdapter::DrawTriangle(
    const TransformComponent& transform,
    const Color4& color
) {
    const float size = 60.0f * transform.scale.x;

    const Vector2 top{
        transform.position.x,
        transform.position.y - size
    };

    const Vector2 left{
        transform.position.x - size,
        transform.position.y + size
    };

    const Vector2 right{
        transform.position.x + size,
        transform.position.y + size
    };

    ::DrawTriangle(
        top,
        left,
        right,
        ToRaylibColor(color)
    );
}

void RaylibRenderAdapter::DrawRectangle(
    const TransformComponent& transform,
    const Color4& color
) {
    const float width = 120.0f * transform.scale.x;
    const float height = 80.0f * transform.scale.y;

    ::DrawRectangle(
        static_cast<int>(transform.position.x - width / 2.0f),
        static_cast<int>(transform.position.y - height / 2.0f),
        static_cast<int>(width),
        static_cast<int>(height),
        ToRaylibColor(color)
    );
}

void RaylibRenderAdapter::DrawDebugAABB(
    const AABB& aabb,
    const Color4& color
) {
    const Vector3 center{
        aabb.center.x,
        aabb.center.y,
        aabb.center.z
    };

    const Vector3 size{
        aabb.halfSize.x * 2.0f,
        aabb.halfSize.y * 2.0f,
        aabb.halfSize.z * 2.0f
    };

    rlDisableBackfaceCulling();
    rlSetLineWidth(2.0f);

    DrawCubeWiresV(center, size, ToRaylibColor(color));

    rlSetLineWidth(1.0f);
    rlEnableBackfaceCulling();
}

void RaylibRenderAdapter::DrawDebugSphere(
    const Vec3& center,
    float radius,
    const Color4& color
) {
    const Vector3 sphereCenter{
        center.x,
        center.y,
        center.z
    };

    rlDisableBackfaceCulling();
    rlSetLineWidth(2.0f);

    DrawSphereWires(
        sphereCenter,
        radius,
        16,
        16,
        ToRaylibColor(color)
    );

    rlSetLineWidth(1.0f);
    rlEnableBackfaceCulling();
}

GPUTexture RaylibRenderAdapter::CreateTexture(
    const TextureData& data
) {
    GPUTexture gpuTexture{};

    if (
        data.Pixels.empty() ||
        data.Width <= 0 ||
        data.Height <= 0
    ) {
        return gpuTexture;
    }

    int format = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    if (data.Channels == 1) {
        format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    }
    else if (data.Channels == 2) {
    format = RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
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
    GPUShaderProgram program{};

    if (vertexSource.empty() || fragmentSource.empty()) {
        return program;
    }

    program.ID = rlLoadShaderCode(
        vertexSource.c_str(),
        fragmentSource.c_str()
    );

    if (program.ID == 0) {
        TraceLog(
            LOG_ERROR,
            "Failed to compile shader program"
        );
    }
    else {
        TraceLog(
            LOG_INFO,
            "Shader program compiled successfully. ID: %u",
            program.ID
        );
    }

    return program;
}

GPUMesh RaylibRenderAdapter::UploadMesh(
    const MeshData& data
) {
    GPUMesh gpuMesh{};

    if (data.Vertices.empty() || data.Indices.empty()) {
        return gpuMesh;
    }

    if (data.Vertices.size() > 65535) {
        TraceLog(
            LOG_ERROR,
            "Mesh has too many vertices for unsigned short indices"
        );

        return gpuMesh;
    }

    std::vector<unsigned short> indices16;
    indices16.reserve(data.Indices.size());

    for (unsigned int index : data.Indices) {
        indices16.push_back(
            static_cast<unsigned short>(index)
        );
    }

    gpuMesh.IndexCount = indices16.size();

    gpuMesh.VAO = rlLoadVertexArray();
    rlEnableVertexArray(gpuMesh.VAO);

    gpuMesh.VBO = rlLoadVertexBuffer(
        data.Vertices.data(),
        static_cast<int>(
            data.Vertices.size() * sizeof(Vertex)
        ),
        false
    );

    gpuMesh.EBO = rlLoadVertexBufferElement(
        indices16.data(),
        static_cast<int>(
            indices16.size() * sizeof(unsigned short)
        ),
        false
    );

    rlSetVertexAttribute(
        0,
        3,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        static_cast<int>(offsetof(Vertex, Position))
    );
    rlEnableVertexAttribute(0);

    rlSetVertexAttribute(
        1,
        3,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        static_cast<int>(offsetof(Vertex, Normal))
    );
    rlEnableVertexAttribute(1);

    rlSetVertexAttribute(
        2,
        2,
        RL_FLOAT,
        false,
        sizeof(Vertex),
        static_cast<int>(offsetof(Vertex, TexCoords))
    );
    rlEnableVertexAttribute(2);

    rlDisableVertexArray();

    return gpuMesh;
}

// Сохраняем прежний способ вызова.
// Он рассчитывает матрицу и передаёт её новому методу.
void RaylibRenderAdapter::DrawMesh(
    const GPUMesh& mesh,
    const GPUTexture& texture,
    const GPUShaderProgram& shader,
    const TransformComponent& transform
) {
    DrawPreparedMesh(
        mesh,
        texture,
        shader,
        BuildModelMatrix(transform)
    );
}

// Вызывать из главного потока.
// Матрица объекта уже рассчитана до начала отрисовки.
void RaylibRenderAdapter::DrawPreparedMesh(
    const GPUMesh& mesh,
    const GPUTexture& texture,
    const GPUShaderProgram& shader,
    const PreparedModelMatrix& model
) {
    if (
        mesh.VAO == 0 ||
        mesh.IndexCount == 0 ||
        shader.ID == 0
    ) {
        return;
    }

    rlEnableShader(shader.ID);

    if (texture.ID != 0) {
        rlActiveTextureSlot(0);
        rlEnableTexture(texture.ID);

        const int textureLocation = rlGetLocationUniform(
            shader.ID,
            "texture0"
        );

        const int textureSlot = 0;

        rlSetUniform(
            textureLocation,
            &textureSlot,
            RL_SHADER_UNIFORM_INT,
            1
        );
    }

    const Matrix matModel = ToRaylibMatrix(model);
    const Matrix matView = rlGetMatrixModelview();
    const Matrix matProjection = rlGetMatrixProjection();

    const Matrix matModelView = MatrixMultiply(
        matModel,
        matView
    );

    const Matrix matMVP = MatrixMultiply(
        matModelView,
        matProjection
    );

    const int mvpLocation = rlGetLocationUniform(
        shader.ID,
        "mvp"
    );

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

void RaylibRenderAdapter::DeleteMesh(
    const GPUMesh& mesh
) {
    if (mesh.VAO != 0) {
        rlUnloadVertexArray(mesh.VAO);
    }

    if (mesh.VBO != 0) {
        rlUnloadVertexBuffer(mesh.VBO);
    }

    if (mesh.EBO != 0) {
        rlUnloadVertexBuffer(mesh.EBO);
    }
}

void RaylibRenderAdapter::DeleteTexture(
    const GPUTexture& texture
) {
    if (texture.ID != 0) {
        rlUnloadTexture(texture.ID);
    }
}

void RaylibRenderAdapter::DeleteShaderProgram(
    const GPUShaderProgram& shader
) {
    if (shader.ID != 0) {
        rlUnloadShaderProgram(shader.ID);
    }
}

void RaylibRenderAdapter::SetCamera(
    const CameraComponent& camera
) {
    activeCamera = camera;
}

void RaylibRenderAdapter::Begin3D() {
    Camera3D camera{};

    camera.position = Vector3{
        activeCamera.position.x,
        activeCamera.position.y,
        activeCamera.position.z
    };

    camera.target = Vector3{
        activeCamera.position.x + activeCamera.forward.x,
        activeCamera.position.y + activeCamera.forward.y,
        activeCamera.position.z + activeCamera.forward.z
    };

    camera.up = Vector3{
        activeCamera.up.x,
        activeCamera.up.y,
        activeCamera.up.z
    };

    camera.fovy = activeCamera.fov;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camera);
}

void RaylibRenderAdapter::End3D() {
    EndMode3D();
}

void RaylibRenderAdapter::DrawTextOnScreen(
    const std::string& text,
    int x,
    int y,
    int fontSize
) {
    DrawText(
        text.c_str(),
        x,
        y,
        fontSize,
        BLACK
    );
}

void RaylibRenderAdapter::EndFrame() {
    EndDrawing();
}

void RaylibRenderAdapter::Shutdown() {
    CloseWindow();
}