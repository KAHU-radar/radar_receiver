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

/*
Napi::Value RadarInfoWrapper::Bar(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected one number argument").ThrowAsJavaScriptException();
        return env.Null();
    }
    int x = info[0].As<Napi::Number>().Int32Value();
    int result = radar_info_->bar(x);
    return Napi::Number::New(env, result);
}
*/

Napi::Function RadarInfoWrapper::GetClass(Napi::Env env) {
    return DefineClass(env, "RadarInfo", {
      //InstanceMethod("bar", &RadarInfoWrapper::Bar)
    });
}
