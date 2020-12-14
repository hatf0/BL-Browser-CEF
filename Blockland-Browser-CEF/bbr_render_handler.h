#ifndef BBR_RENDER_HANDLER_H
#define BBR_RENDER_HANDLER_H
#include "Torque.h"
#include "osr_render_handler_win.h"
#include "glapi.h"
#include "glext.h"

#define DEBUGGL n = BL_glGetError(); if (n) Printf("GL Error [%u]: %u", __LINE__, n);

class BBR : public OsrRenderHandlerWin {
	friend class BlockBrowser;
public:
	BBR(const OsrRendererSettings& settings, HGLRC context, HDC device);
	~BBR();

	virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, void* shared_handle);
	virtual void OnPaint(CefRefPtr< CefBrowser > browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h);
	virtual void Render();

	HGLRC mainContext;
	HDC mainDevice;
protected:
	int textureID;
	HGLRC ourContext;
	GLuint glBuffer;

	IMPLEMENT_REFCOUNTING(BBR);
};


// void swapBuffersHook();

#endif