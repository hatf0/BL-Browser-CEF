#define ConsoleMethod(type, function) type ts_##function(ADDR obj, int argc, const char* argv[])
#define RegisterMethod(name, description, argmin, argmax) tsf_AddConsoleFunc(NULL, NULL, #name, ts_##name, description, argmin, argmax);
#define BBR BlockBrowserRender
#include <Windows.h>
#include "RedoBlHooks.hpp"
#include "torque.hpp"
#include <string>
#include <map>
#include <psapi.h>
#include <shlwapi.h>

#include <include/base/cef_bind.h>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <include/wrapper/cef_helpers.h>
#include <include/cef_base.h>

#include <include/cef_browser.h>
#include <include/cef_client.h>

#include <include/wrapper/cef_closure_task.h>
#include "glapi.h"
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

#include <thread>
//std::thread BlockBrowserThread;
HANDLE BlockBrowserThread;
int* globalTextureID = new int;
char moduleDir[MAX_PATH];
TextureObject** smTable;
CefRefPtr<CefBrowser> browser;
void ts_DumpTextures(ADDR obj, int argc, const char* argv[]);
int texID;
char* texBuffer;
bool* isDirty = new bool;
class BlockBrowser : public CefApp {
public:
	BlockBrowser() {};
private:
	IMPLEMENT_REFCOUNTING(BlockBrowser);
};
#define DEBUGGL n = BL_glGetError(); if (n) BlPrintf("GL Error [%u]: %u", __LINE__, n);
class BBR : public CefRenderHandler {
public:
	BBR(int w, int h) : width(w), height(h) {
		// Gen buffer.
	}
	~BBR() {
		// Remove buffers.
	}
	virtual CefRefPtr<CefAccessibilityHandler> GetAccessibilityHandler() { return nullptr; }

	virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
		//rect = CefRect(0, 0, width, height);
		return false;
	}
	virtual bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& info) {
		info.rect = CefRect(0, 0, width, height);
		info.device_scale_factor = 1.0;
		return true;
	}
	virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int vx, int vy, int& sx, int& sy) {
		return false;
	}
	virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
		CEF_REQUIRE_UI_THREAD();
		BlPrintf("GetViewRect(0, 0, %u, %u)", width, height);
		rect = CefRect(0, 0, width, height);
	}
	virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, void* shared_handle) { }
	virtual void OnImeCompositionRangeChanged(CefRefPtr< CefBrowser > browser, const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds) { }
	virtual void OnPaint(CefRefPtr< CefBrowser > browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h) {
		CEF_REQUIRE_UI_THREAD();
		BlPrintf("On paint. ><");
		if (*globalTextureID != 0) {
			BlPrintf("On paint.");
			*isDirty = true;
			if (BL_glBindBufferARB) {
				BL_glBindBufferARB(GL_TEXTURE_BUFFER, *texBuffer);
				BL_glBufferDataARB(GL_TEXTURE_BUFFER, width * height * 4, buffer, GL_DYNAMIC_DRAW);
			}
			else {
				memcpy(texBuffer, buffer, w * h * 4);
			}

			//dirty = true;
			//texBuffer_0 = (char*)buffer;

			//sdirty = true;
			//bloader_printf("OnPaint");
		}
	}
	virtual void OnPopupShow(CefRefPtr< CefBrowser > browser, bool show) { }
	virtual void OnPopupSize(CefRefPtr< CefBrowser > browser, const CefRect& rect) { }
	virtual void OnScrollOffsetChanged(CefRefPtr< CefBrowser > browser, double x, double y) { }
	virtual void OnTextSelectionChanged(CefRefPtr< CefBrowser > browser, const CefString& selected_text, const CefRange& selected_range) {}
	virtual void OnVirtualKeyboardRequested(CefRefPtr< CefBrowser > browser, CefRenderHandler::TextInputMode input_mode) { }
	virtual bool StartDragging(CefRefPtr< CefBrowser > browser, CefRefPtr< CefDragData > drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y) { return false; }
	void UpdateDragCursor(CefRefPtr< CefBrowser > browser, CefRenderHandler::DragOperation operation) { }
	void UpdateResolution(int w, int h) {
		if (w * h * 4 > 67108864 || w * h * 4 < 262144)
			return;
		//memset(texBuffer, 0, w * h * 4);
		width = w;
		height = h;
	}
private:
	IMPLEMENT_REFCOUNTING(BBR);
	int width, height;
};

class BrowserClient : public CefClient, public CefLifeSpanHandler, public CefLoadHandler {
public:
	BrowserClient(CefRenderHandler* ptr) : handler(ptr) {}
	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
	CefRefPtr<CefRenderHandler> GetRenderHandler() override { return handler; }
	void OnAfterCreated(CefRefPtr<CefBrowser> browser) {
		browser_id = browser->GetIdentifier();
	}
	bool DoClose(CefRefPtr<CefBrowser> browser) {
		if (browser->GetIdentifier() == browser_id) {
			closing = true;
		}
		return false;
	}
	void OnBeforeClose(CefRefPtr<CefBrowser> browser) { BlPrintf("OnBeforeClose"); }
	void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
		BlPrintf("OnLoadend");
		browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
		loaded = true;
	}
	void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& failedUrl, CefString& errorText) {
		BlPrintf("OnLoadError");
		browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
		loaded = true;
	}
	void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
		BlPrintf("OnLoadingStateChange");
		browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
	}
	void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) { BlPrintf("OnLoadStart"); }
	bool closeAllowed() const { return closing; };
	bool isLoaded() const { return loaded; };
private:
	int browser_id;
	bool closing = false;
	bool loaded = false;
	CefRefPtr<CefRenderHandler> handler;
	IMPLEMENT_REFCOUNTING(BrowserClient);

};
CefRefPtr<BrowserClient> browser_client;
CefRefPtr<BBR> renderHandler;
//void resizeWindow(ADDR obj, int argc, const char* argv[]) {
ConsoleMethod(void, resizeWindow) {
	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	renderHandler->UpdateResolution(width, height);
	browser->GetHost()->WasResized();
}
//void mouseMove(ADDR obj, int argc, const char* argv[]) {
ConsoleMethod(void, mouseMove) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	browser->GetHost()->SendMouseMoveEvent(*event, false);
	delete event;
}
ConsoleMethod(void, mouseClick) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	int clickType = atoi(argv[3]);
	browser->GetHost()->SendMouseClickEvent(*event, (cef_mouse_button_type_t)clickType, false, 1);
	browser->GetHost()->SendMouseClickEvent(*event, (cef_mouse_button_type_t)clickType, true, 1);
	delete event;
}
ConsoleMethod(void, mouseWheel) {
	CefMouseEvent* event = new CefMouseEvent();
	event->x = atoi(argv[1]);
	event->y = atoi(argv[2]);
	int deltaX = atoi(argv[3]);
	int deltaY = atoi(argv[4]);
	browser->GetHost()->SendMouseWheelEvent(*event, deltaX, deltaY);
	delete event;
}
ConsoleMethod(void, keyboardEvent) {
	CefKeyEvent* event = new CefKeyEvent();
	event->character = argv[1][0];
	event->modifiers = atoi(argv[2]);
	browser->GetHost()->SendKeyEvent(*event);
	delete event;
}
// Crashed on bindToTexture call, could be related to hook rewriting texture, look into future.
ConsoleMethod(void, bindToTexture) {
	TextureObject* tex = smTable[0];
	int cnt = 0;
	for (; tex; tex = tex->next) {
		if (tex == NULL) continue;
		cnt++;
		if (!tex->texFileName || !tex->texGLName) {
			BlPrintf("An error has occurred at texture %u", cnt);
			continue;
		}
		if (strstr(tex->texFileName, "-asterisk") && !strstr(tex->texFileName, "icons")) {
			*globalTextureID = tex->texGLName;
			BL_glBindTexture(GL_TEXTURE_2D, *globalTextureID);
			BL_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_BGRA, GL_UNSIGNED_BYTE, texBuffer);

			BlPrintf("Found.");
			return;
		}
	}

}
ConsoleMethod(void, setTextureID) {
	*globalTextureID = atoi(argv[1]);
	BL_glBindTexture(GL_TEXTURE_2D, *globalTextureID);
	int n;
	DEBUGGL;
	BL_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_BGRA, GL_UNSIGNED_BYTE, texBuffer);
	DEBUGGL;
}
ConsoleMethod(void, setBrowserPage) {
	browser->GetMainFrame()->LoadURL(argv[1]);
}
bool* doBreakPtr = new bool(true);
DWORD WINAPI threadLoop(void* lpParam) {
	bool* doBreak = (bool*)lpParam;
	CefMainArgs args(GetModuleHandle(nullptr));
	CefSettings settings;
	settings.multi_threaded_message_loop = false;
	//settings.no_sandbox = true;
	CefString(&settings.browser_subprocess_path).FromASCII("C:/Program Files (x86)/Steam/steamapps/common/Blockland/cefclient.exe");
	CefString(&settings.user_agent).FromASCII("Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36 Blockland/r2033 (Torque Game Engine/1.3)");
	if (!CefInitialize(args, settings, new BlockBrowser(), nullptr)) {
		BlPrintf("Failed to initialize Blockland Browser.");
		//doBreak = false;
		return 0;
	}
	renderHandler = new BBR(2048, 2048);
	browser_client = new BrowserClient(renderHandler);
	CefWindowInfo window_info;
	window_info.windowless_rendering_enabled = true;
	window_info.SetAsWindowless(nullptr);
	window_info.x = 0;
	window_info.y = 0;
	window_info.width = 2048;
	window_info.height = 2048;
	window_info.shared_texture_enabled = false;
	CefBrowserSettings browser_settings;
	browser_settings.windowless_frame_rate = 60;

	browser = CefBrowserHost::CreateBrowserSync(window_info, browser_client.get(), "https://www.google.com", browser_settings, NULL, NULL);

	browser->GetHost()->WasResized();
	while (true) {
		if (*doBreak == false) {
			browser->GetHost()->CloseBrowser(true);
			CefShutdown();
			BlPrintf("Blockland browser shut down.");
			return 0;
		}
		CefDoMessageLoopWork();
		Sleep(1);
	}
	//CefRunMessageLoop();
	return 0;
}
struct Point2I {
	int x, y;
};
/*
BlFunctionDef(char, __cdecl, Platform__initWindow, int a, int b, const char* name);

char __cdecl Platform__initWindowHook(int a, int b, const char* name);
BlFunctionHookDef(Platform__initWindow);
bool globalInit = false;
char __cdecl Platform__initWindowHook(int a, int b, const char* name) {
	for (int k = 0; k < 5; k++) BlPrintf("Attempting a hook");
	if (!globalInit) {
		BlockBrowserThread = CreateThread(NULL, 0, threadLoop,doBreakPtr, 0, NULL);
		globalInit = true;
		Platform__initWindowHookOff();
	}
	return Platform__initWindow(a, b, name);
}
*/
BlFunctionDef(void, , swapBuffers);
void swapBuffersHook();
BlFunctionHookDef(swapBuffers);

void swapBuffersHook() { // 55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4 64 A3 ? ? ? ? 80 3D
	int n = 0;
	if (*globalTextureID && *isDirty) {
		*isDirty = false;
		BL_glBindTexture(GL_TEXTURE_2D, *globalTextureID);
		DEBUGGL;
		BL_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2048, 2048, GL_BGRA, GL_UNSIGNED_BYTE, texBuffer);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		DEBUGGL;


	}
	swapBuffersHookOff();
	swapBuffers();
	swapBuffersHookOn();
}

ConsoleMethod(void, setDirty) {
	browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);

	*isDirty = true;
}
ConsoleMethod(void, randomizeBuffer) {
	for (int x = 0; x < 2048; x++) {
		for (int y = 0; y < 2048; y++) {
			for (int z = 0; z < 4; z++) {
				texBuffer[2048 * 4 * x + y * 4 + z] = rand();
			}
		}
	}
}



bool init() {
	*isDirty = false;
	//8B 0D ? ? ? ? 8B F8 89 3D
	BlInit;
	BlPrintf("Ultralight Bind attaching.");
	int n;
	BlScanHex(n, "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 8B F8");
	n += 2;
	BlPrintf("Address of texturemanager is %08X", *(int*)n);
	smTable = (TextureObject**)*(int*)n;
	if (!tsf_InitInternal()) return false;
	tsf_AddConsoleFunc(NULL, NULL, "dumpTextures", ts_DumpTextures, "() - Dumps textures in the TextureManager.", 1, 1);
	RegisterMethod(resizeWindow, "resizes window", 3, 3);
	RegisterMethod(mouseClick, "(int x, int y, int button) - Sends a click event at the given location.", 4, 4);
	RegisterMethod(mouseWheel, "(int x, int y, int deltaX, int deltaY) - Sends a scroll event", 5, 5);
	RegisterMethod(keyboardEvent, "(char k, int modifiers) - Sends a keyboard event", 3, 3);
	RegisterMethod(bindToTexture, "() - Attempts to bind the targeted texture.", 1, 1);
	RegisterMethod(setTextureID, "(int id) - Forces the texture id.", 2, 2);
	RegisterMethod(setDirty, "() - Sets the screen dirty.", 1, 1);
	RegisterMethod(randomizeBuffer, "() - Debugging purpose.", 1, 1);
	RegisterMethod(setBrowserPage, "(string url) - Loads the given url.", 2, 2);
	*globalTextureID = 0;
	texBuffer = new char[4096 * 4096 * 4];
	BlPrintf("Attempting to initialize CEF.");
	for (int i = 0; i < 16; i++)
		BlPrintf("Reached line %u", __LINE__);
	initGL();
	for (int i = 0; i < 16; i++)
		BlPrintf("Reached line %u", __LINE__);

	//BlScanFunctionHex(Platform__initWindow, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 51 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 83 3D");
	//Platform__initWindowHookOn();
	BlockBrowserThread = CreateThread(NULL, 0, threadLoop, doBreakPtr, 0, NULL);
	//55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 51 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 83 3D
	BlScanFunctionHex(swapBuffers, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4 64 A3 ? ? ? ? 80 3D");
	swapBuffersHookOn();
	//BlockBrowserThread = CreateThread(NULL, 0, threadLoop, doBreakPtr, 0, NULL);

	return true;
}

void ts_DumpTextures(ADDR obj, int argc, const char* argv[]) {
	BlPrintf("Attempting to dump textures.");
	unsigned int cnt = 0;
	TextureObject* tex = smTable[0];
	for (; tex; tex = tex->next) {
		if (tex == NULL) continue;
		cnt++;
		if (!tex->texFileName || !tex->texGLName) {

			BlPrintf("An error has occurred at texture %u", cnt);
			continue;
		}

		BlPrintf("Texture [%u][%u]: %s", cnt, tex->texGLName, tex->texFileName);
	}
	/**for (TextureObject* tex = ptr; tex; tex = tex->next) {
		BlPrintf("Attempting to read address at %08X", tex);
		cnt++;
		if (cnt == 1) continue;
		//if (cnt > 1000) break;
		if (!tex->texFileName || !tex->texGLName) {
			BlPrintf("An error has occurred at texture %u", cnt);
			continue;
		}
		BlPrintf("Texture [%u]: %s", cnt, tex->texFileName); // Crashing here in game.
	}**/
}
bool deinit() {
	if (*doBreakPtr) {
		*doBreakPtr = false;
		//BlockBrowserThread.join();
		WaitForSingleObject(BlockBrowserThread, INFINITE);
	}
	delete doBreakPtr;
	free(texBuffer);
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