#pragma once
#include<memory>
#include "logger/Logging.hpp"
#include "application/ConcurrentQueue.hpp"
#include "scene/Mesh.hpp"

class RenderAdapterI
{
public:
	virtual ~RenderAdapterI() = default;

	virtual void init(const size_t sceneSize) = 0;
	virtual void draw(ConcurrentQueue<BindHostMeshDataEvent>& bindHostMeshDataEventQueue) = 0;
};
