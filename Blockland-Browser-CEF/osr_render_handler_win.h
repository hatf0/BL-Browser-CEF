#ifndef OSR_RENDER_HANDLER_WIN_H
#define OSR_RENDER_HANDLER_WIN_H
#include "Torque.h"
#include <include/base/cef_weak_ptr.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>
#include <include/cef_render_handler.h>
#include "osr_renderer_settings.h"


extern uint64_t GetTimeNow();

extern TextureObject** smTable;
extern std::shared_ptr<HGLRC> mainGLContext;
extern std::shared_ptr<HDC> mainDeviceContext;

class OsrRenderHandlerWin : public CefRenderHandler {
public:
    OsrRenderHandlerWin(const OsrRendererSettings& settings);
    virtual ~OsrRenderHandlerWin();

    virtual void SetBrowser(CefRefPtr<CefBrowser> browser);

    virtual CefRefPtr<CefAccessibilityHandler> GetAccessibilityHandler();
    virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
    virtual bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& info);
    virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int vx, int vy, int& sx, int& sy);
    virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
    virtual void OnScrollOffsetChanged(CefRefPtr< CefBrowser > browser, double x, double y);
    virtual void OnTextSelectionChanged(CefRefPtr< CefBrowser > browser, const CefString& selected_text, const CefRange& selected_range);
    virtual void OnVirtualKeyboardRequested(CefRefPtr< CefBrowser > browser, CefRenderHandler::TextInputMode input_mode);
    virtual bool StartDragging(CefRefPtr< CefBrowser > browser, CefRefPtr< CefDragData > drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y);
    void UpdateDragCursor(CefRefPtr< CefBrowser > browser, CefRenderHandler::DragOperation operation);
    void UpdateResolution(int w, int h);

    // CefRenderHandler callbacks.
    virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show);
    // |rect| must be in pixel coordinates.
    virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect);

    // Used when not rendering with shared textures.
    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList& dirtyRects,
        const void* buffer,
        int width,
        int height) = 0;

    // Used when rendering with shared textures.
    virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList& dirtyRects,
        void* share_handle) = 0;

    bool send_begin_frame() const {
        return settings_.external_begin_frame_enabled;
    }

    // Called by the BeginFrame timer.
    virtual void Render() = 0;

protected:
    int width;
    int height;
    // Called to trigger the BeginFrame timer.
    void Invalidate();


private:
    void TriggerBeginFrame(uint64_t last_time_us, float delay_us);

    // The below members are only accessed on the UI thread.
    const OsrRendererSettings settings_;
    bool begin_frame_pending_;
    CefRefPtr<CefBrowser> browser_;

    // Must be the last member.
    base::WeakPtrFactory<OsrRenderHandlerWin> weak_factory_;

    DISALLOW_COPY_AND_ASSIGN(OsrRenderHandlerWin);
};

void InitRenderHandler();
#endif