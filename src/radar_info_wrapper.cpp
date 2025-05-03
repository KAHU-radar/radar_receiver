#include "radar_info_wrapper.h"
#include "radar_pi.h"
#include "pi_common.h"
#include <string.h>

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

struct ProcessRadarSpokeData {
    int angle;
    int bearing;
    uint8_t* data;
    size_t len;
    int range_meters;
    wxLongLong time;
    GeoPosition pos;
};

struct NotifyData {
    PLUGIN_NAMESPACE::ControlInfo *ctrl;
    PLUGIN_NAMESPACE::RadarControlItem *item;
};

Napi::Value ControlInfoToNode(Napi::Env env, PLUGIN_NAMESPACE::ControlInfo *ctrl) {
    Napi::Object tobj = Napi::Object::New(env);
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
    return tobj;
}

Napi::Value RadarControlItemToNode(Napi::Env env, PLUGIN_NAMESPACE::RadarControlItem *item) {
    Napi::Object vobj = Napi::Object::New(env);
    vobj.Set("value", item->GetValue());
    vobj.Set("state", (int) item->GetState());
    vobj.Set("mod", item->IsModified());
    vobj.Set("min", item->GetMin());
    vobj.Set("max", item->GetMax());
    return vobj;
}

Napi::Value RadarControlInfoItemToNode(Napi::Env env, PLUGIN_NAMESPACE::ControlInfo *ctrl, PLUGIN_NAMESPACE::RadarControlItem *item) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("type", ControlInfoToNode(env, ctrl));
    obj.Set("value", RadarControlItemToNode(env, item));
    return obj;
}

RadarInfoWrapper::RadarInfoWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RadarInfoWrapper>(info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected radar type number").ThrowAsJavaScriptException();
        return;
    }

    radar = new PLUGIN_NAMESPACE::radar_pi(NULL);
    radar->Init();
    radar->m_radar[0] = new PLUGIN_NAMESPACE::RadarInfo(radar, 0);

    process_radar_spoke_fn = NULL;
    if ((info.Length() > 1) && info[1].IsFunction()) {
        process_radar_spoke_fn = Napi::ThreadSafeFunction::New(
            env,
            info[1].As<Napi::Function>(),
            "process_radar_spoke_fn",
            0,                            // Unlimited queue
            1                             // Only one thread will use this initially
        );

        radar->m_radar[0]->SetProcessRadarSpokeFN(
            new PLUGIN_NAMESPACE::ProcessRadarSpokeFN(
                [this](PLUGIN_NAMESPACE::SpokeBearing angle, PLUGIN_NAMESPACE::SpokeBearing bearing,
                       uint8_t* data, size_t len, int range_meters,
                       wxLongLong time, GeoPosition pos) {

                    auto callback = [](Napi::Env env, Napi::Function jsCallback, void* d) {
                        ProcessRadarSpokeData *spoke = (ProcessRadarSpokeData *) d;
                        uint8_t* jsdata = new uint8_t[spoke->len];
                        memcpy(jsdata, spoke->data, spoke->len);
                        auto finalizer = [](Napi::Env env, void* dataPtr) {
                            delete[] static_cast<uint8_t*>(dataPtr);
                        };
                        jsCallback.Call({
                          Napi::Number::New(env, spoke->angle),
                          Napi::Number::New(env, spoke->bearing),
                          Napi::Uint8Array::New(env, spoke->len, Napi::ArrayBuffer::New(env, jsdata, spoke->len, finalizer), 0),
                          Napi::Number::New(env, spoke->range_meters),
                          Napi::Number::New(env, spoke->time.GetValue())
                        });
                        delete spoke;
                    };

                    process_radar_spoke_fn.NonBlockingCall(
                        new ProcessRadarSpokeData{angle, bearing, data, len, range_meters, time, pos},
                        callback);
               }));
    }

    notify_fn = NULL;
    if ((info.Length() > 2) && info[2].IsFunction()) {
        notify_fn = Napi::ThreadSafeFunction::New(
            env,
            info[2].As<Napi::Function>(),
            "notify_fn",
            0,                            // Unlimited queue
            1                             // Only one thread will use this initially
        );

        radar->m_radar[0]->SetNotifyFN(
            new PLUGIN_NAMESPACE::NotifyFN(
                [this](PLUGIN_NAMESPACE::RadarControlItem *item) {
                    ssize_t property_index = GetPropertyIndexByItem(item);
                    if (property_index < 0) return; // This should never happen, we have an item!
                    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
                    auto callback = [](Napi::Env env, Napi::Function jsCallback, void* d) {
                        NotifyData *data = (NotifyData *) d;
                        jsCallback.Call({
                            RadarControlInfoItemToNode(env, data->ctrl, data->item)
                        });
                    };
                    notify_fn.NonBlockingCall(
                        new NotifyData{&dlg->m_ctrl[property_index], dlg->m_button[property_index]->m_item},
                        callback);
               }));
    }

    
    radar->m_settings.radar_count = 1;
    radar->m_radar[0]->m_radar_type = (PLUGIN_NAMESPACE::RadarType) (info[0].As<Napi::Number>().Int32Value());  // modify type of existing radar ?
    radar->StartRadarLocators(0);
    radar->m_radar[0]->Init();
}


void RadarInfoWrapper::Shutdown(const Napi::CallbackInfo& info) {
    radar->m_radar[0]->Shutdown();
    radar->DeInit();
    if (process_radar_spoke_fn) process_radar_spoke_fn.Release();
    if (notify_fn) notify_fn.Release();
}

void RadarInfoWrapper::SetTransmit(const Napi::CallbackInfo& info) {
   if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(info.Env(), "Expected a boolean").ThrowAsJavaScriptException();
        return;
   }
   radar->m_radar[0]->SetTransmit(info[0].As<Napi::Boolean>().Value());
}

Napi::Value RadarInfoWrapper::GetType(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), radar->m_radar[0]->m_radar_type);
}

ssize_t GetPropertyIndexByName(std::string property_name) {
    size_t property_index;
    for (property_index = 0;
         property_index < PLUGIN_NAMESPACE::CT_MAX;
         property_index++) {
        if (control_type_identifiers[property_index] == property_name)
            return (ssize_t) property_index;
    }
    return -1;
}

ssize_t RadarInfoWrapper::GetPropertyIndexByItem(PLUGIN_NAMESPACE::RadarControlItem *item) {
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    size_t property_index;
    for (property_index = 0;
         property_index < PLUGIN_NAMESPACE::CT_MAX;
         property_index++) {
        PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[property_index];
        if (button && (item == button->m_item))
             return property_index;
    }
    return -1;
}

Napi::Value RadarInfoWrapper::SetProperty(const Napi::CallbackInfo& info) {
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(info.Env(), "Expected a string property name and an integer value").ThrowAsJavaScriptException();
        return info.Env().Null();
    }
    ssize_t property_index = GetPropertyIndexByName(info[0].As<Napi::String>().Utf8Value());
    if (property_index < 0) {
        Napi::TypeError::New(info.Env(), "Unknown property name").ThrowAsJavaScriptException();
        return info.Env().Null();
    }
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[property_index];
    if (!button) {
        Napi::TypeError::New(info.Env(), "Property not supported by this radar type").ThrowAsJavaScriptException();
        return info.Env().Null();
    }
    PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[property_index];
    PLUGIN_NAMESPACE::RadarControlItem *item = button->m_item;

    item->Update(info[1].As<Napi::Number>().Int32Value());
}

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
        if (button) {
            result[result_i++] = RadarControlInfoItemToNode(env, &dlg->m_ctrl[i], button->m_item);
       }
    }
    
    return result;
}

Napi::Function RadarInfoWrapper::GetClass(Napi::Env env) {
    return DefineClass(env, "RadarInfo", {
      //InstanceMethod("init", &RadarInfoWrapper::Init),
      InstanceMethod("shutdown", &RadarInfoWrapper::Shutdown),
      InstanceMethod("setTransmit", &RadarInfoWrapper::SetTransmit),
      InstanceMethod("setProperty", &RadarInfoWrapper::SetProperty),
      InstanceMethod("getType", &RadarInfoWrapper::GetType),
      InstanceMethod("getProperties", &RadarInfoWrapper::GetProperties)
    });
}
