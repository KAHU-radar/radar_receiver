#include <napi.h>
#include <wx/wx.h>

Napi::String Hello(const Napi::CallbackInfo& info) {
    wxString msg = "Hello from wxWidgets!";
    return Napi::String::New(info.Env(), msg.ToStdString());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("hello", Napi::Function::New(env, Hello));
    return exports;
}

NODE_API_MODULE(addon, Init)
