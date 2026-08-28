#pragma once
#include <memory>

#include "application/ConcurrentQueue.hpp"
#include "logger/Logging.hpp"
#include "scene/Mesh.hpp"

class RenderAdapterI {
   public:
    virtual ~RenderAdapterI() = default;

    virtual void init(const size_t sceneSize) = 0;
    virtual void draw(ConcurrentQueue<ENG::BindHostMeshDataEvent>& bindHostMeshDataEventQueue) = 0;
};
