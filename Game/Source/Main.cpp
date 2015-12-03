#include "BsApplication.h"
#include "BsCrashHandler.h"
#include "BsCoreThread.h"
#include "BsFileSerializer.h"
#include "BsGameSettings.h"
#include "BsFileSystem.h"
#include "BsResources.h"
#include "BsResourceManifest.h"
#include "BsPrefab.h"
#include "BsSceneObject.h"
#include "BsSceneManager.h"

void runApplication();

#if BS_PLATFORM == BS_PLATFORM_WIN32
#include <windows.h>

using namespace BansheeEngine;

int CALLBACK WinMain(
	_In_  HINSTANCE hInstance,
	_In_  HINSTANCE hPrevInstance,
	_In_  LPSTR lpCmdLine,
	_In_  int nCmdShow
	)
{
	CrashHandler::startUp();

	__try
	{
		runApplication();
	}
	__except (gCrashHandler().reportCrash(GetExceptionInformation()))
	{
		PlatformUtility::terminate(true);
	}

	CrashHandler::shutDown();

	return 0;
}
#endif // End BS_PLATFORM

using namespace BansheeEngine;

void runApplication()
{
	FileDecoder fd(GAME_SETTINGS_PATH);
	SPtr<GameSettings> gameSettings = std::static_pointer_cast<GameSettings>(fd.decode());

	if (gameSettings == nullptr)
		gameSettings = bs_shared_ptr_new<GameSettings>();

	unsigned int resolutionWidth = 200;
	unsigned int resolutionHeight = 200;

	if (!gameSettings->fullscreen)
	{
		resolutionWidth = gameSettings->resolutionWidth;
		resolutionHeight = gameSettings->resolutionHeight;
	}

	RENDER_WINDOW_DESC renderWindowDesc;
	renderWindowDesc.videoMode = VideoMode(resolutionWidth, resolutionHeight);
	renderWindowDesc.title = toString(gameSettings->titleBarText);
	renderWindowDesc.fullscreen = false;
	renderWindowDesc.hidden = gameSettings->fullscreen;

	Application::startUp(renderWindowDesc, RenderAPIPlugin::DX11);

	if (gameSettings->fullscreen)
	{
		if (gameSettings->useDesktopResolution)
		{
			const VideoModeInfo& videoModeInfo = RenderAPI::getVideoModeInfo();
			const VideoOutputInfo& primaryMonitorInfo = videoModeInfo.getOutputInfo(0);
			const VideoMode& selectedVideoMode = primaryMonitorInfo.getDesktopVideoMode();

			RenderWindowPtr window = gApplication().getPrimaryWindow();
			window->setFullscreen(gCoreAccessor(), selectedVideoMode);

			resolutionWidth = selectedVideoMode.getWidth();
			resolutionHeight = selectedVideoMode.getHeight();
		}
		else
		{
			resolutionWidth = gameSettings->resolutionWidth;
			resolutionHeight = gameSettings->resolutionHeight;

			VideoMode videoMode(resolutionWidth, resolutionHeight);

			RenderWindowPtr window = gApplication().getPrimaryWindow();
			window->show(gCoreAccessor());
			window->setFullscreen(gCoreAccessor(), videoMode);
		}
	}

	gameSettings->useDesktopResolution = false; // Not relevant after first startup

	// TODO - Save full video mode
	gameSettings->resolutionWidth = resolutionWidth;
	gameSettings->resolutionHeight = resolutionHeight;

	FileEncoder fe(GAME_SETTINGS_PATH);
	fe.encode(gameSettings.get());

	Path resourceManifestPath = GAME_RESOURCES_PATH + GAME_RESOURCE_MANIFEST_NAME;

	ResourceManifestPtr manifest;
	if (FileSystem::exists(resourceManifestPath))
	{
		Path resourcesPath = FileSystem::getWorkingDirectoryPath();
		resourcesPath.append(APP_ROOT);

		manifest = ResourceManifest::load(resourceManifestPath, resourcesPath);

		gResources().registerResourceManifest(manifest);
	}

	HPrefab mainScene = static_resource_cast<Prefab>(gResources().loadFromUUID(gameSettings->mainSceneUUID));
	if (mainScene != nullptr)
	{
		HSceneObject root = mainScene->instantiate();
		HSceneObject oldRoot = gSceneManager().getRootNode();

		gSceneManager()._setRootNode(root);
		oldRoot->destroy();
	}

	Application::instance().runMainLoop();
	Application::shutDown();
}