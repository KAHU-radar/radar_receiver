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
/*
Napi::Value RadarInfoWrapper::SetProperty(const Napi::CallbackInfo& info) {
 ControlType controlType;
 for (controlType = CT_MAX-1; controlType > CT_NONE; controlType--) {
     control_type_identifiers[controlType]
 }


 bool SetControlValue(ControlType controlType, RadarControlItem &item, RadarControlButton *button);
*/
Napi::Value RadarInfoWrapper::GetProperties(const Napi::CallbackInfo& info) { 
   PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    if (!dlg) {
        Napi::Env env = info.Env();
        return env.Null();
    }
    size_t nr_controls = 0;
    for (size_t i = 0; i < ARRAY_SIZE(dlg->m_ctrl); i++) {
        if (dlg->m_button[i]) {
            nr_controls++;
        }
    }
 
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env, nr_controls);

    size_t result_i = 0;

    for (size_t i = 0; i < PLUGIN_NAMESPACE::CT_MAX; i++) {
        PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[i];
        PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[i];
        if (button) {
            PLUGIN_NAMESPACE::RadarControlItem *item = button->m_item;

            Napi::Object obj = Napi::Object::New(env);
            Napi::Object tobj = Napi::Object::New(env);
            obj.Set("type", tobj);
            Napi::Object vobj = Napi::Object::New(env);
            obj.Set("value", vobj);

            vobj.Set("value", item->GetValue());
            vobj.Set("state", (int) item->GetState());
            vobj.Set("mod", item->IsModified());
            vobj.Set("min", item->GetMin());
            vobj.Set("max", item->GetMax());
        
            tobj.Set("id", control_type_identifiers[ctrl->type]);
            tobj.Set("title", control_type_titles[ctrl->type]);
            Napi::Array auto_names = Napi::Array::New(env, ctrl->autoValues);
            for (size_t j = 0; j < ctrl->autoValues; j++) {
                auto_names[j] = ctrl->autoNames[j];
            }
            tobj.Set("autoNames", auto_names);

            tobj.Set("hasOff", ctrl->hasOff);
            tobj.Set("hasAutoAdjustable", ctrl->hasAutoAdjustable);
            tobj.Set("defaultValue", ctrl->defaultValue);
            tobj.Set("minValue", ctrl->minValue);
            tobj.Set("maxValue", ctrl->maxValue);
            tobj.Set("minAdjustValue", ctrl->minAdjustValue);
            tobj.Set("maxAdjustValue", ctrl->maxAdjustValue);
            tobj.Set("stepValue", ctrl->stepValue);
            Napi::Array names = Napi::Array::New(env, ctrl->names ? ctrl->nameCount : 0);
            if (ctrl->names) {
                for (size_t j = 0; j < ctrl->nameCount; j++) {
                    names[j] = ctrl->names[j];
                }
            }
            tobj.Set("names", names);
            tobj.Set("unit", ctrl->unit);

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
