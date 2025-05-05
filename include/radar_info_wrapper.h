#ifndef RADAR_INFO_WRAPPER_H
#define RADAR_INFO_WRAPPER_H

#include <napi.h>
#include "radar_pi.h"
#include "RadarInfo.h"
#include "RadarControlItem.h"

class RadarInfoWrapper : public Napi::ObjectWrap<RadarInfoWrapper> {
public:
    static Napi::Function GetClass(Napi::Env env);

    RadarInfoWrapper(const Napi::CallbackInfo& info);
//    void Init(const Napi::CallbackInfo& info);
    void Shutdown(const Napi::CallbackInfo& info);
    void SetProperty(const Napi::CallbackInfo& info);
    void SetTransmit(const Napi::CallbackInfo& info);
    Napi::Value GetType(const Napi::CallbackInfo& info);
    Napi::Value GetProperty(const Napi::CallbackInfo& info);
    Napi::Value GetPropertyType(const Napi::CallbackInfo& info);
    Napi::Value GetProperties(const Napi::CallbackInfo& info);
    ssize_t GetPropertyIndexByItem(PLUGIN_NAMESPACE::RadarControlItem *item);

private:
    PLUGIN_NAMESPACE::radar_pi* radar;
    Napi::ThreadSafeFunction process_radar_spoke_fn;
    Napi::ThreadSafeFunction notify_fn;
};

#endif
