#include "osr_render_handler_win.h"
#include "Torque.h"

std::shared_ptr<HGLRC> mainGLContext;
std::shared_ptr<HDC> mainDeviceContext;
TextureObject** smTable;

OsrRenderHandlerWin::OsrRenderHandlerWin(const OsrRendererSettings& settings)
    : settings_(settings),
    begin_frame_pending_(false),
    weak_factory_(this), width(2048), height(2048) {
    CEF_REQUIRE_UI_THREAD();
}

OsrRenderHandlerWin::~OsrRenderHandlerWin() {
    CEF_REQUIRE_UI_THREAD();
}

void OsrRenderHandlerWin::SetBrowser(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    browser_ = browser;
    if (browser_ && settings_.external_begin_frame_enabled) {
        // Start the BeginFrame timer.
        Invalidate();
    }
}

void OsrRenderHandlerWin::Invalidate() {
    CEF_REQUIRE_UI_THREAD();
    if (begin_frame_pending_) {
        // The timer is already running.
        return;
    }

    // Trigger the BeginFrame timer.
    CHECK_GT(settings_.begin_frame_rate, 0);
    const float delay_us = (1.0 / double(settings_.begin_frame_rate)) * 1000000.0;
    TriggerBeginFrame(0, delay_us);
}

void OsrRenderHandlerWin::TriggerBeginFrame(uint64_t last_time_us,
    float delay_us) {
    if (begin_frame_pending_ && !settings_.external_begin_frame_enabled) {
        // Render immediately and then wait for the next call to Invalidate() or
        // On[Accelerated]Paint().
        begin_frame_pending_ = false;
        Render();
        return;
    }

    const auto now = GetTimeNow();
    float offset = now - last_time_us;
    if (offset > delay_us) {
        offset = delay_us;
    }

    if (!begin_frame_pending_) {
        begin_frame_pending_ = true;
    }

    // Trigger again after the necessary delay to maintain the desired frame rate.
    CefPostDelayedTask(TID_UI,
        base::Bind(&OsrRenderHandlerWin::TriggerBeginFrame,
            weak_factory_.GetWeakPtr(), now, delay_us),
        int64(offset / 1000.0));

    if (settings_.external_begin_frame_enabled && browser_) {
        // We're running the BeginFrame timer. Trigger rendering via
        // On[Accelerated]Paint().
        browser_->GetHost()->SendExternalBeginFrame();
    }
}

CefRefPtr<CefAccessibilityHandler> OsrRenderHandlerWin::GetAccessibilityHandler() {
    CEF_REQUIRE_UI_THREAD();
    return nullptr;
}

bool OsrRenderHandlerWin::GetRootScreenRect(CefRefPtr<CefBrowser>, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    return false;
}

bool OsrRenderHandlerWin::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& info) {
    CEF_REQUIRE_UI_THREAD();
    CefRect view_rect;
    GetViewRect(browser, view_rect);

    info.rect = view_rect;
    info.available_rect = view_rect;
    info.device_scale_factor = 1.0;
    return true;
}

void OsrRenderHandlerWin::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    CEF_REQUIRE_UI_THREAD();
    Printf("GetViewRect(0, 0, %u, %u)", width, height);
    rect = CefRect(0, 0, width, height);
}

bool OsrRenderHandlerWin::GetScreenPoint(CefRefPtr<CefBrowser> browser, int vx, int vy, int& sx, int& sy) {
    return false;
}

void OsrRenderHandlerWin::OnPopupShow(CefRefPtr< CefBrowser > browser, bool show) { }
void OsrRenderHandlerWin::OnPopupSize(CefRefPtr< CefBrowser > browser, const CefRect& rect) { }
void OsrRenderHandlerWin::OnScrollOffsetChanged(CefRefPtr< CefBrowser > browser, double x, double y) { }
void OsrRenderHandlerWin::OnTextSelectionChanged(CefRefPtr< CefBrowser > browser, const CefString& selected_text, const CefRange& selected_range) {}
void OsrRenderHandlerWin::OnVirtualKeyboardRequested(CefRefPtr< CefBrowser > browser, CefRenderHandler::TextInputMode input_mode) { }
bool OsrRenderHandlerWin::StartDragging(CefRefPtr< CefBrowser > browser, CefRefPtr< CefDragData > drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y) { return false; }
void OsrRenderHandlerWin::UpdateDragCursor(CefRefPtr< CefBrowser > browser, CefRenderHandler::DragOperation operation) { }
void OsrRenderHandlerWin::UpdateResolution(int w, int h) {
    if (w * h * 4 > 67108864 || w * h * 4 < 262144)
        return;
    //memset(texBuffer, 0, w * h * 4);
    width = w;
    height = h;
}

void InitRenderHandler() {
    Printf("%s - Initializing render handler", PROJECT);
    DWORD n;
    n = ScanFunc("\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x8B\xF8", "xx????????xx????????x????xx????xx");
    n += 2;
    smTable = ((TextureObject**)*(int*)n);

    Printf("%s - Address of texturemanager is %08X", PROJECT, *(int*)n);
    Printf("%s - Initialized render handler", PROJECT);
}