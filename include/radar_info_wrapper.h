#ifndef RADAR_INFO_WRAPPER_H
#define RADAR_INFO_WRAPPER_H

#include <napi.h>
#include "radar_pi.h"
#include "RadarInfo.h"

class RadarInfoWrapper : public Napi::ObjectWrap<RadarInfoWrapper> {
public:
    static Napi::Function GetClass(Napi::Env env);

    RadarInfoWrapper(const Napi::CallbackInfo& info);

private:
//    Napi::Value Bar(const Napi::CallbackInfo& info);
    PLUGIN_NAMESPACE::RadarInfo* radar_info_;
};

#endif
