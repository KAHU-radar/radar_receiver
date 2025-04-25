#include "radar_info_wrapper.h"

RadarInfoWrapper::RadarInfoWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RadarInfoWrapper>(info) {
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(info.Env(), "Expected radar type number").ThrowAsJavaScriptException();
        return;
    }

    radar_info_ = new PLUGIN_NAMESPACE::RadarInfo(
        NULL,
        info[0].As<Napi::Number>().Int32Value());
}

Napi::Function RadarInfoWrapper::GetClass(Napi::Env env) {
    return DefineClass(env, "RadarInfo", {
      //InstanceMethod("bar", &RadarInfoWrapper::Bar)
    });
}
