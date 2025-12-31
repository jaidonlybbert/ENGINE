#ifndef ENG_GUI
#define ENG_GUI
#include<functional>

class Gui {
public:
	void registerDrawCall(std::function<void(void)> drawCall);
	void drawGui();
	
private:
	std::vector<std::function<void(void)>> drawCalls;
};
#endif
