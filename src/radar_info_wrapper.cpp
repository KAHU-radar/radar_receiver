#include "radar_info_wrapper.h"
#include "radar_pi.h"

std::string control_type_identifiers[] = {
    "CT_NONE",
#define CONTROL_TYPE(x, y) #x,
#include "ControlType.inc"
#undef CONTROL_TYPE
};
std::string control_type_titles[] = {
    "CT_NONE",
#define CONTROL_TYPE(x, y) y,
#include "ControlType.inc"
#undef CONTROL_TYPE
};
     
RadarInfoWrapper::RadarInfoWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RadarInfoWrapper>(info) {
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(info.Env(), "Expected radar type number").ThrowAsJavaScriptException();
        return;
    }

    radar = new PLUGIN_NAMESPACE::radar_pi(NULL);
    radar->Init();
    radar->m_radar[0] = new PLUGIN_NAMESPACE::RadarInfo(radar, 0);
    radar->m_settings.radar_count = 1;
    radar->m_radar[0]->m_radar_type = (PLUGIN_NAMESPACE::RadarType) (info[0].As<Napi::Number>().Int32Value());  // modify type of existing radar ?
    radar->StartRadarLocators(0);
    radar->m_radar[0]->Init();
}


void RadarInfoWrapper::Shutdown(const Napi::CallbackInfo& info) {
    radar->m_radar[0]->Shutdown();
    radar->DeInit();
}

Napi::Value RadarInfoWrapper::GetType(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), radar->m_radar[0]->m_radar_type);
}

Napi::Value RadarInfoWrapper::GetProperties(const Napi::CallbackInfo& info) {
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    if (!dlg) {
        Napi::Env env = info.Env();
        return env.Null();
    }
    size_t nr_controls = 0;
    for (size_t i = 0; i < ARRAY_SIZE(dlg->m_ctrl); i++) {
        if (dlg->m_ctrl[i].type != PLUGIN_NAMESPACE::CT_NONE) {
            nr_controls++;
        }
    }
 
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env, nr_controls);

    size_t result_i = 0;

    for (size_t i = 0; i < ARRAY_SIZE(dlg->m_ctrl); i++) {
        PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[i];
        if (ctrl->type != PLUGIN_NAMESPACE::CT_NONE) {
            Napi::Object obj = Napi::Object::New(env);

            obj.Set("id", control_type_identifiers[ctrl->type]);
            obj.Set("title", control_type_titles[ctrl->type]);
            Napi::Array auto_names = Napi::Array::New(env, ctrl->autoValues);
            for (size_t j = 0; j < ctrl->autoValues; j++) {
                auto_names[j] = ctrl->autoNames[j];
            }
            obj.Set("autoNames", auto_names);

            obj.Set("hasOff", ctrl->hasOff);
            obj.Set("hasAutoAdjustable", ctrl->hasAutoAdjustable);
            obj.Set("defaultValue", ctrl->defaultValue);
            obj.Set("minValue", ctrl->minValue);
            obj.Set("maxValue", ctrl->maxValue);
            obj.Set("minAdjustValue", ctrl->minAdjustValue);
            obj.Set("maxAdjustValue", ctrl->maxAdjustValue);
            obj.Set("stepValue", ctrl->stepValue);
            Napi::Array names = Napi::Array::New(env, ctrl->names ? ctrl->nameCount : 0);
            if (ctrl->names) {
                for (size_t j = 0; j < ctrl->nameCount; j++) {
                    names[j] = ctrl->names[j];
                }
            }
            obj.Set("names", names);
            obj.Set("unit", ctrl->unit);

            result[result_i++] = obj;
       }
    }
    
    return result;
}

Napi::Function RadarInfoWrapper::GetClass(Napi::Env env) {
    return DefineClass(env, "RadarInfo", {
      //InstanceMethod("init", &RadarInfoWrapper::Init),
      InstanceMethod("shutdown", &RadarInfoWrapper::Shutdown),
      InstanceMethod("getType", &RadarInfoWrapper::GetType),
      InstanceMethod("getProperties", &RadarInfoWrapper::GetProperties)
    });
}
