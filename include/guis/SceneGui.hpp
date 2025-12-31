#pragma once

#include<glm/glm.hpp>

namespace ENG {
	class Node;
	class SceneState;
}

struct GUICameraSettings {
	glm::vec3 position{0.f};
	glm::vec3 direction{0.f};
	float zoom{0.f};
};

struct GUISettings {
	GUICameraSettings camera;
	bool showSettings{true};
};


class SceneGui
{
public:
	GUISettings settings;
	void MySaveFunction();
	void DrawNodeTree(ENG::Node* node);
	void drawGui(ENG::SceneState& sceneState);
};
