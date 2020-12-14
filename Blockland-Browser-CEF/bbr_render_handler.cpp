#include "bbr_render_handler.h"

BBR::BBR(const OsrRendererSettings& settings, HGLRC ctx, HDC device)
	: OsrRenderHandlerWin(settings), mainDevice(device), ourContext(ctx) {
	CEF_REQUIRE_UI_THREAD();
	textureID = settings.texture_id;

	if (!device || !ctx) {
		Printf("Could not get context / device");
		return;
	}

	if (!BL_wglMakeCurrent(device, ctx)) {
		Printf("Could not set GL context");
		return;
	}

	glBuffer = 0;
	BL_glGenBuffers(1, &glBuffer);
	BL_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glBuffer);
	BL_glBufferStorage(GL_PIXEL_UNPACK_BUFFER, width*height*4, NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT);
	//BL_wglMakeCurrent(NULL, NULL);
}

BBR::~BBR() {
	CEF_REQUIRE_UI_THREAD();
	Printf("Deleting render thread");
	BL_glDeleteBuffers(1, &glBuffer);
}

// not used for this
void BBR::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, void* shared_handle) { 
}

void BBR::OnPaint(CefRefPtr< CefBrowser > browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h) {
	CEF_REQUIRE_UI_THREAD();
	//Printf("On paint. ><");
	/*
	if (!BL_wglMakeCurrent(mainDevice, ourContext)) {
		Printf("Could not set GL context");
		return;
	}
	*/
	if (textureID != 0 && glBuffer) {
		//Printf("On paint.");
		int n = 0;
		BL_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
	}

	// BL_wglMakeCurrent(NULL, NULL);
}
// never reached
void BBR::Render() {
	if (textureID != 0 && glBuffer) {
		int n = 0;
		BL_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glBuffer);
		DEBUGGL;
		BL_glBindTexture(GL_TEXTURE_2D, textureID);
		DEBUGGL;
		BL_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		DEBUGGL;
		BL_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		DEBUGGL;
		BL_glGenerateMipmap(GL_TEXTURE_2D);
		DEBUGGL;
	}
}