#ifndef DX_RENDERER_H
#define DX_RENDERER_H

#include "Torque.h"
#include "osr_render_handler_win.h"
#include "d3d11.h"

class BrowserLayer : public d3d11::Layer {
public:
	explicit BrowserLayer(const std::shared_ptr<d3d11::Device>& device);

	void render(const std::shared_ptr<d3d11::Context>& ctx) OVERRIDE;

	void on_paint(void* share_handle);

	// After calling on_paint() we can query the texture size.
	std::pair<uint32_t, uint32_t> texture_size() const;
	
	std::shared_ptr<d3d11::FrameBuffer> frame_buffer_;

	DISALLOW_COPY_AND_ASSIGN(BrowserLayer);
};

class DXRenderer : public OsrRenderHandlerWin {
public:
	DXRenderer(const OsrRendererSettings& settings);
	~DXRenderer();

	virtual void SetBrowser(CefRefPtr<CefBrowser> browser);
	virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, void* shared_handle);
	virtual void OnPaint(CefRefPtr< CefBrowser > browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h);

private:

	void Render() override;
	void* glInterop;
	//GLuint texBuf;
	std::shared_ptr<d3d11::Device> device_;
	std::shared_ptr<BrowserLayer> browser_layer_;
	std::shared_ptr<d3d11::Composition> composition_;
	uint64_t start_time_;
	IMPLEMENT_REFCOUNTING(DXRenderer);
};

#endif