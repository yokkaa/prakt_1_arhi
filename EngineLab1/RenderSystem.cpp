#include "RenderSystem.h"

#include "RenderAdapter.h"
#include "ResourceManager.h"
#include "RenderPreparation.h"

void RenderSystem::Update(
    World& world,
    RenderAdapter& renderer,
    ResourceManager& resourceManager,
    float deltaTime,
    JobSystem& jobs,
    bool parallel
) {
    (void)deltaTime;

    transforms.clear();
    items.clear();

    // Этап 1. Главный поток собирает данные объектов.
    // Обращения к миру и менеджеру ресурсов остаются здесь.
    for (Entity entity : world.GetEntities()) {
        if (
            !world.HasTransform(entity) ||
            !world.HasMeshRenderer(entity)
        ) {
            continue;
        }

        const MeshRendererComponent& component =
            world.GetMeshRenderer(entity);

        auto mesh = resourceManager.LoadMeshAsync(
            component.meshPath,
            renderer
        );

        auto material = resourceManager.LoadMaterial(
            component.materialPath,
            renderer
        );

        if (
            !mesh ||
            !material ||
            !material->Texture ||
            !material->Shader
        ) {
            continue;
        }

        transforms.push_back(
            world.GetTransform(entity)
        );

        items.push_back({
            mesh->GPUHandle,
            material->Texture->GPUHandle,
            material->Shader->GPUHandle
        });
    }

    // Этап 2. Рассчитываем матрицы объектов.
    //
    // Пока ресурсы загружаются, считаем на главном потоке:
    // так кадр не будет ждать задачи рендера,
    // поставленные в общую очередь за чтением файлов.
    const bool useJobs =
        parallel && !resourceManager.HasPendingLoads();

    lastJobCount = PrepareModelMatrices(
        transforms,
        matrices,
        jobs,
        useJobs
    );

    // Этап 3. Все задачи расчёта завершены.
    // Главный поток рисует объекты в прежнем порядке.
    for (std::size_t i = 0; i < items.size(); ++i) {
        const DrawItem& item = items[i];

        renderer.DrawPreparedMesh(
            item.mesh,
            item.texture,
            item.shader,
            matrices[i]
        );
    }
}