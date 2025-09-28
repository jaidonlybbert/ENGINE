#ifndef ENG_GUI
#define ENG_GUI
#include<glm/glm.hpp>
struct GUICameraSettings {
	glm::vec3 position{0.f};
	glm::vec3 direction{0.f};
	float zoom{0.f};
};

struct GUISettings {
	GUICameraSettings camera;
	bool showSettings{true};
};
#endif
