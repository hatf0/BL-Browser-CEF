#include "cef_app.h"
#include "Torque.h"

CefRefPtr<BlockBrowser> app;

void BlockBrowser::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {
	command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
}

BlockBrowser::BlockBrowser() : started_(false), useExperimental(false), BlockBrowserThread(NULL), textureID(0), device(NULL), context(NULL) {
	Printf("%s - Initializing app", PROJECT);
	doBreakPtr = CefOwnPtr<bool>(new bool(true));
	isDirty = CefOwnPtr<bool>(new bool(false));
}

BlockBrowser::~BlockBrowser() {
	if (started_) {
		stop();
	}
}

void BlockBrowser::setTextureID(int id) {
	if (!useExperimental) {
		if (started_) {
			Printf("Cannot update the texture ID while the render thread is running :(");
		}
		else {
			textureID = id;
		}
	}
}

bool BlockBrowser::getDirty() const {
	return *isDirty;
}

bool BlockBrowser::start(bool experimental) {
	useExperimental = experimental;
	if (textureID == 0) {
		Printf("%s - Please set a texture id before starting CEF", PROJECT);
		return false;
	}

	if (!isGLReady() && !experimental) {
		Printf("%s - GL is not ready");
		return false;
	}

	BlockBrowserThread = std::make_shared<HANDLE>(CreateThread(NULL, 0, BlockBrowser::threadLoop, this->doBreakPtr.get(), 0, NULL)); // ugly
	started_ = true;
	return true;
}

bool BlockBrowser::started() const {
	return started_;
}

DWORD WINAPI BlockBrowser::threadLoop(void* arg) {
	bool* stop = (bool*)arg;
	app->init();
	while (true) {
		if (*stop == false) {
			app->deinit();
			Printf("Blockland browser shut down.");
			return 0;
		}
		CefDoMessageLoopWork();
	}
	return 0;
}

bool BlockBrowser::init() {
	Printf("%s - Initializing CEF", PROJECT);

	CefMainArgs args(GetModuleHandleA("Blockland.exe"));
	CefSettings settings;
	// settings.log_severity = cef_log_severity_t::LOGSEVERITY_DEBUG;
	//settings.multi_threaded_message_loop = true;
	//settings.no_sandbox = true;
	settings.external_message_pump = true;
	settings.windowless_rendering_enabled = true;
	CefString(&settings.browser_subprocess_path).FromASCII("C:/Program Files (x86)/Steam/steamapps/common/Blockland/CefClient.exe");
	CefString(&settings.user_agent).FromASCII("Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36 Blockland/r2033 (Torque Game Engine/1.3)");
	CefString(&settings.log_file).FromASCII("C:/Program Files (x86)/Steam/steamapps/common/Blockland/debug.log");

	if (!CefInitialize(args, settings, this, nullptr)) {
		Printf("Failed to initialize Blockland Browser.");
		//doBreak = false;
		return 0;
	}

	OsrRendererSettings renderSettings;
	CefWindowInfo window_info;
	CefBrowserSettings browser_settings;

	/* to-do: investigate DirectX renderer for GPU accel / avoiding multiple copies */
	if (useExperimental) {
		window_info.external_begin_frame_enabled = true;
		window_info.shared_texture_enabled = true;
		renderSettings.begin_frame_rate = 60;
		renderSettings.external_begin_frame_enabled = true;
		renderSettings.shared_texture_enabled = true;
		renderHandler = new DXRenderer(renderSettings);
	}
	else {
		renderSettings.texture_id = textureID;
		browser_settings.windowless_frame_rate = 60;
		window_info.shared_texture_enabled = false;
		renderHandler = new BBR(renderSettings, *context, *device);
	}

	if (!renderHandler) {
		Printf("This shouldn't happen");
		return false;
	}

	browser_client = new BrowserClient(renderHandler);
	window_info.windowless_rendering_enabled = true;
	window_info.SetAsWindowless(nullptr);
	window_info.x = 0;
	window_info.y = 0;
	window_info.width = 2048;
	window_info.height = 2048;

	browser = CefBrowserHost::CreateBrowserSync(window_info, browser_client.get(), "https://www.google.com", browser_settings, NULL, NULL);
	if (browser) {
		browser->GetHost()->WasResized();
		renderHandler->SetBrowser(browser);
	}
	else {
		Printf("Failed to create browser");
		return false;
	}

	return true;
}

bool BlockBrowser::deinit() {
	if (!started_) {
		Printf("CEF was not running");
		return false;
	}

	/* Make sure we terminate the subprocesses in a clean way */
	if (browser) {
		CefString msgName;
		msgName.FromASCII("KILL_subprocess");
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create(msgName);
		browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
		browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
		browser->GetHost()->CloseBrowser(true);
	}

	while(!browser_client->isClosed()) {
		CefDoMessageLoopWork();
		Sleep(50);
	}

	Printf("Shutting CEF down");
	CefShutdown();
}

bool BlockBrowser::stop() {
	*app->doBreakPtr = false;
	WaitForSingleObject(*BlockBrowserThread, INFINITE);
	started_ = false;
	return true;
}