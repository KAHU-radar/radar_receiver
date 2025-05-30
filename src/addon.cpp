#include <napi.h>
#include <wx/string.h>
#include "radar_info_wrapper.h"

#include <wx/init.h>
#include <mutex>

bool EnsureWxInitialized() {
    static std::once_flag initFlag;
    static bool initialized = false;

    std::call_once(initFlag, []() {
//        if (wxApp::GetInstance() == nullptr) {
            initialized = wxInitialize();
//        }
    });

    return initialized;
}


Napi::Array GetRadarTypes(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env, PLUGIN_NAMESPACE::RT_MAX);
    
    size_t i = 0;
    
    #define DEFINE_RADAR(t, n, s, l, a, b, c, d) do { \
      Napi::Object obj = Napi::Object::New(env); \
      obj.Set("type", #t); \
      obj.Set("name", wxString(n).mb_str(wxConvUTF8).data());  \
      obj.Set("spokes", s); \
      obj.Set("spokeLength", l); \
      result[i++] = obj; \
    } while (0);

    #include "RadarType.h"

    return result;
}

Napi::String Hello(const Napi::CallbackInfo& info) {
    wxString msg = "Hello from wxWidgets!";
    return Napi::String::New(info.Env(), msg.ToStdString());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    EnsureWxInitialized();
    exports.Set("hello", Napi::Function::New(env, Hello));
    exports.Set("getRadarTypes", Napi::Function::New(env, GetRadarTypes));
    exports.Set("RadarInfo", RadarInfoWrapper::GetClass(env));
    return exports;
}

NODE_API_MODULE(addon, Init)
