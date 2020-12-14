#include <memory>
#include <include/cef_app.h>
#include "osr_render_handler_win.h"
#include "bbr_render_handler.h"
#include "dx_renderer.h"
#include "browser_client.h"

class BlockBrowser : public CefApp {
public:
	BlockBrowser();
	~BlockBrowser();

	virtual void OnBeforeCommandLineProcessing(
		const CefString& process_type,
		CefRefPtr<CefCommandLine> command_line) override;

	void setTextureID(int id);
	void setDevice(HDC dev) { device = std::make_shared<HDC>(dev); }
	void setContext(HGLRC ctx) { context = std::make_shared<HGLRC>(ctx); }
	bool isGLReady() { return (device != NULL && context != NULL); }
	bool getDirty() const;
	void setDirty() { *isDirty = true;  }
	bool start(bool experimental = false);
	bool started() const;
	bool stop();
	CefRefPtr<CefBrowser> getBrowser() { return browser; }
	CefRefPtr<BrowserClient> getBrowserClient() {
		return browser_client;
	}
	CefRefPtr<OsrRenderHandlerWin> getRenderHandler() {
		return renderHandler;
	}

private:
	static DWORD WINAPI threadLoop(void* arg);
	bool init();
	bool deinit();

	int textureID;
	bool started_;
	bool useExperimental;
	std::shared_ptr<HDC> device;
	std::shared_ptr<HGLRC> context;
	std::shared_ptr<HANDLE> BlockBrowserThread;
	CefOwnPtr<bool> isDirty;
	CefOwnPtr<bool> doBreakPtr;
	CefRefPtr<CefBrowser> browser;
	CefRefPtr<BrowserClient> browser_client;
	CefRefPtr<OsrRenderHandlerWin> renderHandler;
	IMPLEMENT_REFCOUNTING(BlockBrowser);
};

extern CefRefPtr<BlockBrowser> app;
