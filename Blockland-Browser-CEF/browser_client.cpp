#include "browser_client.h"

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
	browser_id = browser->GetIdentifier();
}

bool BrowserClient::DoClose(CefRefPtr<CefBrowser> browser) {
	Printf("Closing for %d (we are %d)", browser->GetIdentifier(), browser_id);
	if (browser->GetIdentifier() == browser_id) {
		closing = true;
	}
	return false;
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) { 
	Printf("OnBeforeClose"); 
	closed = true;
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
	Printf("OnLoadend");
	browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
	loaded = true;
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& failedUrl, CefString& errorText) {
	Printf("OnLoadError");
	browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
	loaded = true;
}

void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
	Printf("OnLoadingStateChange");
	browser->GetHost()->Invalidate(CefBrowserHost::PaintElementType::PET_VIEW);
}

void BrowserClient::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) { Printf("OnLoadStart"); }
bool BrowserClient::closeAllowed() const { return closing; };
bool BrowserClient::isLoaded() const { return loaded; };
bool BrowserClient::isClosed() const { return closed; }