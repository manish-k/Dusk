#pragma once

namespace dusk
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		//virtual void update(float dt);
	private:

	};

	Application* CreateApplication(int argc, char** argv);
}