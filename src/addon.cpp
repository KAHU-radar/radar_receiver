#include <napi.h>
#include <wx/wx.h>
#include <wx/string.h>
#include "radar_info_wrapper.h"

Napi::Array GetRadarTypes(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env, PLUGIN_NAMESPACE::RT_MAX);

    for (size_t i = 0; i < PLUGIN_NAMESPACE::RT_MAX; ++i) {
        Napi::Object obj = Napi::Object::New(env);

        #define DEFINE_RADAR(t, n, s, l, a, b, c, d) \
          obj.Set("type", #t); \
          obj.Set("name", wxString(n).mb_str(wxConvUTF8).data());  \
          obj.Set("spokes", s); \
          obj.Set("spokeLength", l);
        #include "RadarType.h"
        
        result[i] = obj;
    }

    return result;
}

Napi::String Hello(const Napi::CallbackInfo& info) {
    wxString msg = "Hello from wxWidgets!";
    return Napi::String::New(info.Env(), msg.ToStdString());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("hello", Napi::Function::New(env, Hello));
    exports.Set("getRadarTypes", Napi::Function::New(env, GetRadarTypes));
    exports.Set("RadarInfo", RadarInfoWrapper::GetClass(env));
    return exports;
}

NODE_API_MODULE(addon, Init)
