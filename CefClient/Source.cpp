#include <include/base/cef_bind.h>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <include/wrapper/cef_helpers.h>
#include <include/cef_base.h>

#include <include/cef_browser.h>
#include <include/cef_client.h>

#pragma comment(lib, "libcef.lib")
#pragma comment(lib, "libcef_dll_wrapper.lib")

// Program entry-point function.
class BlockBrowser : public CefApp, public CefRenderProcessHandler {
public:
    BlockBrowser() {};
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE {
        return this;
    }

    virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override {
        command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
    }

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)  {
        auto args = message->GetArgumentList();
        if (message->GetName() == "KILL_subprocess")
        {
            delete this;
            std::exit(EXIT_FAILURE);
        }
        return false;
    }
private:
    IMPLEMENT_REFCOUNTING(BlockBrowser);
};

int main(int argc, char* argv[]) {
    CefMainArgs main_args(GetModuleHandle(NULL));
  
    // Optional implementation of the CefApp interface.
    CefRefPtr<BlockBrowser> app(new BlockBrowser);

    // Execute the sub-process logic. This will block until the sub-process should exit.
    return CefExecuteProcess(main_args, app.get(), nullptr);
}