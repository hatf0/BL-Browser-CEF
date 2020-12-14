#include "dx_renderer.h"
#include "glapi.h"
#include <tests/cefclient/browser/osr_d3d11_win.h>

BrowserLayer::BrowserLayer(const std::shared_ptr<d3d11::Device>& device)
	: d3d11::Layer(device, true /* flip */) {
	frame_buffer_ = std::make_shared<d3d11::FrameBuffer>(device_);
}

void BrowserLayer::render(const std::shared_ptr<d3d11::Context>& ctx) {
	// Use the base class method to draw our texture.
	render_texture(ctx, frame_buffer_->texture());
}

void BrowserLayer::on_paint(void* share_handle) {
	frame_buffer_->on_paint(share_handle);
}

std::pair<uint32_t, uint32_t> BrowserLayer::texture_size() const {
	const auto texture = frame_buffer_->texture();
	return std::make_pair(texture->width(), texture->height());
}


DXRenderer::DXRenderer(const OsrRendererSettings& settings) 
	: OsrRenderHandlerWin(settings) {
}

DXRenderer::~DXRenderer() {
	if (device_) {
		device_.reset();
	}
}

void DXRenderer::SetBrowser(CefRefPtr<CefBrowser> browser) {
	device_ = d3d11::Device::create();
	if (!device_) {
		Printf("Failed to create new D3D11 device");
		return;
	}

	Printf("Got adapter: %s", device_->adapter_name().c_str());
	glInterop = BL_wglDXOpenDeviceNV(*device_);

	//BL_glGenBuffers(1, &texBuf);
	// Create the browser layer.
	browser_layer_ = std::make_shared<BrowserLayer>(device_);
	// Set up the composition.
	composition_ = std::make_shared<d3d11::Composition>(device_, width, height);
	composition_->add_layer(browser_layer_);
	browser_layer_->move(0.0f, 0.0f, 1.0f, 1.0f);
	start_time_ = GetTimeNow();
	

	OsrRenderHandlerWin::SetBrowser(browser);
}

void DXRenderer::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, void* shared_handle) { 
	CEF_REQUIRE_UI_THREAD();
	Printf("hit accelerated paint!");
	if (type == PET_VIEW) {
		browser_layer_->on_paint(shared_handle);
		/*
		if (*globalTextureID) {
			HANDLE lock;
			BL_wglDXLockObjectsNV(glInterop, 1, &lock);
			BL_wglDXRegisterObjectNV(glInterop, shared_handle, *globalTextureID, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
			BL_wglDXUnlockObjectsNV(glInterop, 1, &lock);
		}
		*/
	}
	else {

	}

	Render();
}

void DXRenderer::OnPaint(CefRefPtr< CefBrowser > browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int w, int h) {
}

void DXRenderer::Render() {
	const auto t = (GetTimeNow() - start_time_) / 1000000.0;
	composition_->tick(t);

	auto ctx = device_->immedidate_context();
	const auto texture_size = browser_layer_->texture_size();

	// Resize the composition and swap chain to match the texture if necessary.
	composition_->resize(!send_begin_frame(), texture_size.first,
		texture_size.second);
	// Render the scene.
	composition_->render(ctx);
	ctx->flush();
}