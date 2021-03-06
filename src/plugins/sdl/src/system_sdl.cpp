#include "system_sdl.h"
#include <SDL.h>
#include <fstream>
#include "halley/core/api/halley_api_internal.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include "sdl_rw_ops.h"
#include "halley/core/graphics/window.h"
#include "halley/os/os.h"
#include "sdl_window.h"
#include "sdl_gl_context.h"
#include "input_sdl.h"
#include "halley/support/logger.h"

using namespace Halley;

void SystemSDL::init()
{
	// SDL version
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	std::cout << ConsoleColour(Console::GREEN) << "\nInitializing SDL..." << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Compiled: " << ConsoleColour(Console::DARK_GREY) << int(compiled.major) << "." << int(compiled.minor) << "." << int(compiled.patch) << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Linked: " << ConsoleColour(Console::DARK_GREY) << int(linked.major) << "." << int(linked.minor) << "." << int(linked.patch) << ConsoleColour() << std::endl;

	// Initialize SDL
	if (!SDL_WasInit(0)) {
		if (SDL_Init(0) == -1) {
			throw Exception(String("Exception initializing SDL: ") + SDL_GetError());
		}
	}
	if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1) {
		throw Exception(String("Exception initializing timer: ") + SDL_GetError());
	}
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		throw Exception(String("Exception initializing joystick: ") + SDL_GetError());
	}

	SDL_ShowCursor(SDL_DISABLE);

	// Init clipboard
	clipboard = OS::get().getClipboard();
}

void SystemSDL::deInit()
{
	// Close SDL
	SDL_Quit();
}

Path SystemSDL::getAssetsPath(const Path& gamePath) const
{
	return gamePath / ".." / "assets";
}

Path SystemSDL::getUnpackedAssetsPath(const Path& gamePath) const
{
	return gamePath / ".." / "assets_unpacked";
}

bool SystemSDL::generateEvents(VideoAPI* video, InputAPI* input)
{
	SDL_Event event;
	SDL_PumpEvents();
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
		switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_TEXTINPUT:
			case SDL_TEXTEDITING:
			case SDL_JOYAXISMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEWHEEL:
			case SDL_FINGERUP:
			case SDL_FINGERDOWN:
			case SDL_FINGERMOTION:
			{
				auto sdlInput = dynamic_cast<InputSDL*>(input);
				if (sdlInput) {
					sdlInput->processEvent(event);
				}
				break;
			}
			case SDL_QUIT:
			{
				std::cout << "SDL_QUIT received." << std::endl;
				return false;
			}
			case SDL_WINDOWEVENT:
			{
				if (video) {
					processVideoEvent(video, event);
				}
				break;
			}
		}
	}
	return true;
}

void SystemSDL::processVideoEvent(VideoAPI* video, const SDL_Event& event)
{
	for (auto& w : windows) {
		if (w->getId() == int(event.window.windowID)) {
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				int x, y;
				SDL_GetWindowPosition(SDL_GetWindowFromID(event.window.windowID), &x, &y);
				w->updateDefinition(w->getDefinition().withPosition(Vector2i(x, y)).withSize(Vector2i(event.window.data1, event.window.data2)));
			} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Maximized));
			} else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Minimized));
			} else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Normal));
			}
		}
	}
}

std::unique_ptr<ResourceDataReader> SystemSDL::getDataReader(String path, int64_t start, int64_t end)
{
	return SDLRWOps::fromPath(path, start, end);
}

std::shared_ptr<Window> SystemSDL::createWindow(const WindowDefinition& windowDef)
{
	initVideo();
	bool openGL = true;

	// Set flags and GL attributes
	auto windowType = windowDef.getWindowType();
	int flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI;
	if (openGL) {
		flags |= SDL_WINDOW_OPENGL;
	}
	if (windowType == WindowType::BorderlessWindow) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	else if (windowType == WindowType::ResizableWindow) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	else if (windowType == WindowType::Fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	// Context options
	if (openGL) {
#if defined(WITH_OPENGL_ES2)
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
#else
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef _DEBUG
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	}

	// Window position
	Vector2i windowSize = windowDef.getSize();
	Vector2i winPos = windowDef.getPosition().get_value_or(getCenteredWindow(windowSize, 0));

	// Create window
	auto sdlWindow = SDL_CreateWindow(windowDef.getTitle().c_str(), winPos.x, winPos.y, windowSize.x, windowSize.y, flags);
	if (!sdlWindow) {
		throw Exception(String("Error creating SDL window: ") + SDL_GetError());
	}

	// Show window
	auto window = std::make_shared<SDLWindow>(sdlWindow);
	window->show();
	window->update(windowDef);
	windows.push_back(window);
	
	return window;
}

void SystemSDL::destroyWindow(std::shared_ptr<Window> window)
{
	for (size_t i = 0; i < windows.size(); ++i) {
		if (windows[i] == window) {
			windows[i]->destroy();
			windows.erase(windows.begin() + i);
			break;
		}
	}
	if (windows.empty()) {
		deInitVideo();
	}
}

Vector2i SystemSDL::getScreenSize(int n) const
{
	initVideo();
	SDL_DisplayMode info;
	if (SDL_GetDesktopDisplayMode(n, &info) == 0) {
		return Vector2i(info.w, info.h);
	} else {
		return Vector2i();
	}
}

Rect4i SystemSDL::getDisplayRect(int screen) const
{
	screen = std::max(0, std::min(screen, SDL_GetNumVideoDisplays() - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);
	return Rect4i(rect.x, rect.y, rect.w, rect.h);
}

Vector2i SystemSDL::getCenteredWindow(Vector2i size, int screen) const
{
	Rect4i rect = getDisplayRect(screen);
	return rect.getTopLeft() + (rect.getSize() - size) / 2;
}

std::unique_ptr<GLContext> SystemSDL::createGLContext()
{
	return std::make_unique<SDLGLContext>(windows[0]->getSDLWindow());
}

void SystemSDL::showCursor(bool show)
{
	SDL_ShowCursor(show ? 1 : 0);
}

std::shared_ptr<ISaveData> SystemSDL::getStorageContainer(SaveDataType type, const String& containerName)
{
	Path dir = saveDir[type];
	if (!containerName.isEmpty()) {
		dir = dir / containerName / ".";
	}

	OS::get().createDirectories(dir);
	return std::make_shared<SDLSaveData>(dir);
}

void SystemSDL::setEnvironment(Environment* env)
{
	for (int i = 0; i < int(SaveDataType::Cache); ++i) {
		SaveDataType type = SaveDataType(i);
		auto dir = env->getDataPath() / toString(type) / ".";
		saveDir[type] = dir;
	}
}

bool SystemSDL::canExit()
{
	return true;
}

std::shared_ptr<IClipboard> SystemSDL::getClipboard() const
{
	return clipboard;
}

void SystemSDL::printDebugInfo() const
{
	std::cout << std::endl << ConsoleColour(Console::GREEN) << "Initializing Video Display...\n" << ConsoleColour();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColour(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColour() << std::endl;
}

void SystemSDL::initVideo() const
{
	if (!videoInit) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
			throw Exception(String("Exception initializing video: ") + SDL_GetError());
		}
		SDL_VideoInit(nullptr);
		printDebugInfo();
		videoInit = true;
	}
}

void SystemSDL::deInitVideo()
{
	if (videoInit) {
		SDL_VideoQuit();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		videoInit = false;
	}
}

SDLSaveData::SDLSaveData(Path dir)
	: dir(dir)
{
}

bool SDLSaveData::isReady() const
{
	return true;
}

Bytes SDLSaveData::getData(const String& path)
{
	Expects (!path.isEmpty());
	return Path::readFile(dir / path);
}

std::vector<String> SDLSaveData::enumerate(const String& root)
{
	auto paths = OS::get().enumerateDirectory(dir);
	std::vector<String> result;
	for (auto& p: paths) {
		auto path = p.toString();
		if (path.startsWith(root)) {
			result.push_back(path);
		}
	}
	return result;
}

void SDLSaveData::setData(const String& path, const Bytes& data, bool commit)
{
	Expects (!path.isEmpty());

	OS::get().createDirectories(dir);
	Path::writeFile(dir / path, data);

	Logger::logDev("Saving \"" + path + "\", " + String::prettySize(data.size()));
}

void SDLSaveData::commit()
{
}
