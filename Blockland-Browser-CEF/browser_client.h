#ifndef BROWSER_CLIENT_H
#define BROWSER_CLIENT_H

#include "Torque.h"
#include <include/cef_client.h>


class BrowserClient : public CefClient, public CefLifeSpanHandler, public CefLoadHandler {
public:
	BrowserClient(CefRenderHandler* ptr) : handler(ptr), browser_id(0) {};
	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
		return this;
	}
	CefRefPtr<CefLoadHandler> GetLoadHandler() override {
		return this;
	}
	CefRefPtr<CefRenderHandler> GetRenderHandler() override {
		return handler;
	}
	void OnAfterCreated(CefRefPtr<CefBrowser> browser);
	bool DoClose(CefRefPtr<CefBrowser> browser);
	void OnBeforeClose(CefRefPtr<CefBrowser> browser);
	void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode);
	void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& failedUrl, CefString& errorText);
	void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward);
	void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
	bool closeAllowed() const;
	bool isLoaded() const;
	bool isClosed() const;
private:
	int browser_id;
	bool closing = false;
	bool closed = false;
	bool loaded = false;
	CefRefPtr<CefRenderHandler> handler;
	IMPLEMENT_REFCOUNTING(BrowserClient);
};
#endif