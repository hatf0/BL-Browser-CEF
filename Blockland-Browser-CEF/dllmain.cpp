#include "Torque.h"
#include "cef_app.h"
#include <string>
#include <map>
#include <psapi.h>
#include <shlwapi.h>
#include <thread>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")
#pragma comment(lib, "D3D11.lib")

#ifdef USE_POLYHOOK
#pragma comment(lib, "PolyHook_2.lib")
#pragma comment(lib, "capstone_dll.lib")
#endif

ConsoleMethod(void, resizeWindow) {
	int width = atoi(argv[1]);
	int height = atoi(argv[2]);

	app->getRenderHandler()->UpdateResolution(width, height);
	app->getBrowser()->GetHost()->WasResized();
}

ConsoleMethod(void, mouseMove) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	app->getBrowser()->GetHost()->SendMouseMoveEvent(*event, false);
	delete event;
}

ConsoleMethod(void, mouseClick) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	int clickType = atoi(argv[3]);
	app->getBrowser()->GetHost()->SendMouseClickEvent(*event, (cef_mouse_button_type_t)clickType, false, 1);
	app->getBrowser()->GetHost()->SendMouseClickEvent(*event, (cef_mouse_button_type_t)clickType, true, 1);
	delete event;
}

ConsoleMethod(void, mouseWheel) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	int deltaX = atoi(argv[3]);
	int deltaY = atoi(argv[4]);
	app->getBrowser()->GetHost()->SendMouseWheelEvent(*event, deltaX, deltaY);
	delete event;
}

ConsoleMethod(void, keyboardEvent) {
	CefKeyEvent* event = new CefKeyEvent();
	event->character = argv[1][0];
	event->modifiers = atoi(argv[2]);
	app->getBrowser()->GetHost()->SendKeyEvent(*event);
	delete event;
}

ConsoleMethod(void, bindToTexture) {
	TextureObject* tex = *smTable;
	int cnt = 0;
	for (; tex; tex = tex->next) {
		if (tex == NULL) continue;
		cnt++;
		if (!tex->texFileName || !tex->texGLName) {
			Printf("An error has occurred at texture %u", cnt);
			continue;
		}
		if (strstr(tex->texFileName, "-asterisk") && !strstr(tex->texFileName, "icons")) {
			Printf("Found.");
			app->setTextureID(tex->texGLName);
			//BL_glBindTexture(GL_TEXTURE_2D, *globalTextureID);
			//BL_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_BGRA, GL_UNSIGNED_BYTE, texBuffer);
			return;
		}
	}
}

ConsoleMethod(void, setTextureID) {
	app->setTextureID(atoi(argv[1]));
}

ConsoleMethod(void, setBrowserPage) {
	app->getBrowser()->GetMainFrame()->LoadURL(argv[1]);
}

ConsoleMethod(bool, initCEF) {
	if (!app->isGLReady()) {
		Printf("Please wait, CEF is trying to get the main GL context.");
		return false;
	}

	return app->start();
}

ConsoleMethod(void, deinitCEF) {
	app->stop();
}

ConsoleMethod(void, setDirty) {
	app->getBrowser()->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
}

ConsoleMethod(void, dumpTextures) {
	Printf("Attempting to dump textures.");
	unsigned int cnt = 0;
	TextureObject* tex = *smTable;
	for (; tex; tex = tex->next) {
		if (tex == NULL) continue;
		cnt++;
		if (!tex->texFileName || !tex->texGLName) {
			Printf("An error has occurred at texture %u", cnt);
			continue;
		}

		Printf("Texture [%u][%u]: %s", cnt, tex->texGLName, tex->texFileName);
	}
}

HDC device = NULL;
HGLRC mainContext = NULL;
ConsoleMethod(void, registerGL) {
	if (!device) { 	// when this is called, we should have a HGLRC
		Printf("%s - Initializing OpenGL", PROJECT);
		initGL();
		if (BL_wglGetCurrentDC && BL_wglGetCurrentContext) {
			device = BL_wglGetCurrentDC();
			mainContext = BL_wglGetCurrentContext();
			if (!device || !mainContext) {
				Printf("could not grab device / context");
			}
			else {
				int gl33_attribs[] = {
					WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
					WGL_CONTEXT_MINOR_VERSION_ARB, 6,
					WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
					0,
				};

				HGLRC newCtx = BL_wglCreateContextAttribsARB(device, mainContext, gl33_attribs);
				if (!newCtx) {
					Printf("Could not make GL context");
				}
				else {
					app->setDevice(device);
					app->setContext(newCtx);
				}
			}
		}
	}
}

processEventsFn oldProcessEvent = NULL;
void __fastcall ProcessEvent(void* this_, int edx, void* evt) {
	if (app->started()) {
		app->getRenderHandler()->Render();
	}	
	if (oldProcessEvent) {
		(oldProcessEvent)(this_, evt);
	}
}

bool init() {
	if (!InitTorqueStuff())
		return false;

	InitRenderHandler();
	app = new BlockBrowser();

	Printf("%s - Registering methods", PROJECT);
	RegisterMethod(dumpTextures, "() - Dumps textures in the TextureManager.", 1, 1);
	RegisterMethod(resizeWindow, "resizes window", 3, 3);
	RegisterMethod(mouseClick, "(int x, int y, int button) - Sends a click event at the given location.", 4, 4);
	RegisterMethod(mouseWheel, "(int x, int y, int deltaX, int deltaY) - Sends a scroll event", 5, 5);
	RegisterMethod(keyboardEvent, "(char k, int modifiers) - Sends a keyboard event", 3, 3);
	RegisterMethod(bindToTexture, "() - Attempts to bind the targeted texture.", 1, 1);
	RegisterMethod(setTextureID, "(int id) - Forces the texture id.", 2, 2);
	RegisterMethod(setDirty, "() - Sets the screen dirty.", 1, 1);
	RegisterMethod(setBrowserPage, "(string url) - Loads the given url.", 2, 2);
	RegisterMethod(registerGL, "() - Hook before we repaint the canvas or do anything..", 1, 1);
	RegisterMethod(initCEF, "()", 1, 1);
	RegisterMethod(deinitCEF, "()", 1, 1);

	const char* cefPackage = "package CEFPackage { function resetCanvas() { registerGL(); Parent::resetCanvas(); } function onExit() { deinitCEF(); Parent::onExit(); } }; activatePackage(CEFPackage);";
	Eval(cefPackage);

	GameInterface* game = (GameInterface*)(ImageBase + 0x3CAFBC);
	oldProcessEvent = (processEventsFn)dAlloc(sizeof(void*));
	Printf("%s - GameInterface: %x", PROJECT, game);
	Printf("%s - Hooking GameInterface::processTimeEvent", PROJECT);
	void** actualVtable = (void**)(*game->vtable); // ugly
	return SwapVTableEntry(actualVtable, 10, &ProcessEvent, (void**)&oldProcessEvent);
}

bool deinit() {
	Printf("%s - deinitializing", PROJECT);
	if (app->started()) {
		app->stop();
	}

	Printf("%s - unhooking GameInterface::processTimeEvent", PROJECT);
	GameInterface* game = (GameInterface*)(ImageBase + 0x3CAFBC);
	if (game) {
		void** actualVtable = (void**)(*game->vtable); // ugly
		if (!SwapVTableEntry(actualVtable, 10, oldProcessEvent, NULL)) {
			Printf("%s - could not unhook GameInterface::processTimeEvent", PROJECT);
		}
	}
	return true;
}
int __stdcall DllMain(HINSTANCE hInstance, unsigned long reason, void* reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		return init();

	case DLL_PROCESS_DETACH:
		return deinit();

	default:
		return true;
	}
}

extern "C" void __declspec(dllexport) __cdecl placeholder(void) { }