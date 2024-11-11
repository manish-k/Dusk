#pragma once

namespace dusk
{
	/**
	 * @brief Base class for all rendering applications
	 */
	class Application
	{
	public:
		Application();
		virtual ~Application();

		/**
		 * @brief Starts the main loop of the rendering
		 * application
		 */
		void Run();

		//virtual void update(float dt);
	private:

	};

	/**
	 * @brief Create the application
	 * @param argc 
	 * @param argv 
	 * @return Pointer to the application instance
	 */
	Application* CreateApplication(int argc, char** argv);
}