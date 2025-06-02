#include "radar_info_wrapper.h"
#include "radar_pi.h"
#include "GuardZone.h"
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

struct ArpaTargetData {
    int target_id;           // 1 target id
    double distance;         // 2 Targ distance
    double bearing;          // 3 Bearing fr own ship.
    std::string bear_unit;   // 4 Brearing unit
    double speed;            // 5 Target speed in knots
    double course;           // 6 Target Course.
    std::string course_unit; // 7 Course ref T // 8 CPA Not used // 9 TCPA Not used
    std::string dist_unit;   // 10 S/D Unit N = knots/Nm or 
    std::string target_name; // 11 Target name
    std::string status;      // 12 Target Status L/Q/T // 13 Ref N/A
};

TimedUpdateThread::TimedUpdateThread(PLUGIN_NAMESPACE::radar_pi* pi) : wxThread(wxTHREAD_JOINABLE) {
    Create(64 * 1024); // Stack size
    m_pi = pi;
    m_shutdown = false;
    m_is_shutdown = true;
    SetPriority(wxPRIORITY_MAX);
    LOG_INFO(wxT("Timed update thread created, prio= %i"), GetPriority());
}

void TimedUpdateThread::Shutdown(void) { m_shutdown = true; }

TimedUpdateThread::~TimedUpdateThread()
    {
        while (!m_is_shutdown) {
            wxMilliSleep(50);
        }
    }

void* TimedUpdateThread::Entry(void) {
    m_is_shutdown = false;
    struct Finalizer {
        volatile bool& ref;
        Finalizer(volatile bool& r) : ref(r) {}
        ~Finalizer() { ref = true; }
    } finalizer(m_is_shutdown);
 
    while (!m_shutdown) {
        m_pi->TimedUpdate();
        m_pi->TimedControlUpdate(); // Maybe call more often?
        wxMilliSleep(500);
    }
    
    return 0;
}

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

Napi::Value RadarControlItemToNode(Napi::Env env, PLUGIN_NAMESPACE::ControlInfo *ctrl, PLUGIN_NAMESPACE::RadarControlItem *item) {
    Napi::Object vobj = Napi::Object::New(env);

    if (ctrl->names && (ctrl->nameCount > 0) && (item->GetValue() < ctrl->nameCount)) {
        vobj.Set("value", ctrl->names[item->GetValue()]);
    } else {
        vobj.Set("value", item->GetValue());
    }
    vobj.Set("state", (int) item->GetState());
    vobj.Set("mod", item->IsModified());
    vobj.Set("min", item->GetMin());
    vobj.Set("max", item->GetMax());
    return vobj;
}

Napi::Value RadarControlInfoItemToNode(Napi::Env env, PLUGIN_NAMESPACE::ControlInfo *ctrl, PLUGIN_NAMESPACE::RadarControlItem *item) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("type", ControlInfoToNode(env, ctrl));
    obj.Set("value", RadarControlItemToNode(env, ctrl, item));
    return obj;
}

RadarInfoWrapper::RadarInfoWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RadarInfoWrapper>(info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected radar type number").ThrowAsJavaScriptException();
        return;
    }
    Napi::Object args = info[0].As<Napi::Object>();

    // if (!args.Has("type") || !args.Get("type").IsNumber()) {
    //     Napi::TypeError::New(env, "Expected args.type to hold a radar type number").ThrowAsJavaScriptException();
    //     return;
    // }

    settings = new Settings();
    if (args.Has("settings") && args.Get("settings").IsObject()) {
        Napi::Object settingsObj = args.Get("settings").As<Napi::Object>();
        settings->LoadFromJson(settingsObj);
    }
    
    radar = new PLUGIN_NAMESPACE::radar_pi(NULL);
    radar->m_pconfig = settings->config;
    radar->Init();
    //radar->m_radar[0] = new PLUGIN_NAMESPACE::RadarInfo(radar, 0);

    process_radar_spoke_fn = NULL;
    if (args.Has("spoke_cb") && args.Get("spoke_cb").IsFunction()) {
        process_radar_spoke_fn = Napi::ThreadSafeFunction::New(
            env,
            args.Get("spoke_cb").As<Napi::Function>(),
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
    if (args.Has("property_cb") && args.Get("property_cb").IsFunction()) {
        notify_fn = Napi::ThreadSafeFunction::New(
            env,
            args.Get("property_cb").As<Napi::Function>(),
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
    
    process_arpa_target_fn = NULL;
    if (args.Has("arpa_cb") && args.Get("arpa_cb").IsFunction()) {
        process_arpa_target_fn = Napi::ThreadSafeFunction::New(
            env,
            args.Get("arpa_cb").As<Napi::Function>(),
            "process_arpa_target_fn",
            0,                            // Unlimited queue
            1                             // Only one thread will use this initially
        );

        radar->SetProcessArpaTargetFN(
            new PLUGIN_NAMESPACE::ProcessArpaTargetFN(
                [this](int target_id,
                       double distance,
                       double bearing,
                       std::string bear_unit,
                       double speed,
                       double course,
                       std::string course_unit,
                       std::string dist_unit,
                       std::string target_name,
                       std::string status
                       ) {
                    auto callback = [](Napi::Env env, Napi::Function jsCallback, void* d) {
                        ArpaTargetData *data = (ArpaTargetData *) d;
                        Napi::Object obj = Napi::Object::New(env);
                        obj.Set("target_id", data->target_id);
                        obj.Set("distance", data->distance);
                        obj.Set("bearing", data->bearing);
                        obj.Set("bear_unit", data->bear_unit);
                        obj.Set("speed", data->speed);
                        obj.Set("course", data->course);
                        obj.Set("course_unit", data->course_unit);
                        obj.Set("dist_unit", data->dist_unit);
                        obj.Set("target_name", data->target_name);
                        obj.Set("status", data->status);
                        jsCallback.Call({obj});
                    };
                    process_arpa_target_fn.NonBlockingCall(
                        new ArpaTargetData{
                         target_id,
                         distance,
                         bearing,
                         bear_unit,
                         speed,
                         course,
                         course_unit,
                         dist_unit,
                         target_name,
                         status},
                        callback);
               }));
    }
 
    //radar->m_settings.radar_count = 1;
    //radar->m_radar[0]->m_radar_type = (PLUGIN_NAMESPACE::RadarType) (args.Get("type").As<Napi::Number>().Int32Value());  // modify type of existing radar ?
    //radar->StartRadarLocators(0);
    //radar->m_radar[0]->Init();
    timed_update_thread = new TimedUpdateThread(radar);
    if (timed_update_thread->Run() != wxTHREAD_NO_ERROR) {
        wxLogError(wxT("unable to start timed update thread"));
    } else {
        LOG_INFO(wxT("timed update thread started"));
    }
}


void RadarInfoWrapper::Shutdown(const Napi::CallbackInfo& info) {
    if (timed_update_thread) {
        timed_update_thread->Shutdown();
        timed_update_thread->Wait();
        delete timed_update_thread;
        timed_update_thread = 0;
    }
    radar->m_radar[0]->Shutdown();
    radar->DeInit();
    if (process_radar_spoke_fn) process_radar_spoke_fn.Release();
    if (notify_fn) notify_fn.Release();
    if (process_arpa_target_fn) process_arpa_target_fn.Release();
    delete settings;
    settings = 0;
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

void RadarInfoWrapper::SetProperty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected a string property name and an integer value").ThrowAsJavaScriptException();
        return;
    }
    ssize_t property_index = GetPropertyIndexByName(info[0].As<Napi::String>().Utf8Value());
    if (property_index < 0) {
        Napi::TypeError::New(env, "Unknown property name").ThrowAsJavaScriptException();
        return;
    }
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[property_index];
    if (!button) {
        Napi::TypeError::New(env, "Property not supported by this radar type").ThrowAsJavaScriptException();
        return;
    }
    PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[property_index];
    PLUGIN_NAMESPACE::RadarControlItem *item = button->m_item;

    if (info[1].IsNumber()) {
        item->Update(info[1].As<Napi::Number>().Int32Value());
    } else if (info[1].IsString() && ctrl->names && (ctrl->nameCount > 0)) {
        int value;
        for (value = 0; value < ctrl->nameCount; value++) {
            if (ctrl->names[value] == info[1].As<Napi::String>().Utf8Value()) {
             item->Update(value);
             return;
           }
        }
       Napi::TypeError::New(env, "Unknown enum value").ThrowAsJavaScriptException();
       return;
   } else {
       Napi::TypeError::New(env, "Expected an integer value for a non-enum property").ThrowAsJavaScriptException();
       return;
   }
}

Napi::Value RadarInfoWrapper::GetProperty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected a string property name").ThrowAsJavaScriptException();
        return env.Null();
    }
    ssize_t property_index = GetPropertyIndexByName(info[0].As<Napi::String>().Utf8Value());
    if (property_index < 0) {
        Napi::TypeError::New(env, "Unknown property name").ThrowAsJavaScriptException();
        return env.Null();
    }
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[property_index];
    if (!button) {
        Napi::TypeError::New(env, "Property not supported by this radar type").ThrowAsJavaScriptException();
        return env.Null();
    }
    PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[property_index];
    PLUGIN_NAMESPACE::RadarControlItem *item = button->m_item;

    return RadarControlItemToNode(env, ctrl, item);
}

Napi::Value RadarInfoWrapper::GetPropertyType(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected a string property name").ThrowAsJavaScriptException();
        return env.Null();
    }
    ssize_t property_index = GetPropertyIndexByName(info[0].As<Napi::String>().Utf8Value());
    if (property_index < 0) {
        Napi::TypeError::New(env, "Unknown property name").ThrowAsJavaScriptException();
        return env.Null();
    }
    PLUGIN_NAMESPACE::ControlsDialog *dlg = radar->m_radar[0]->m_control_dialog;
    PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[property_index];
    if (!button) {
        Napi::TypeError::New(env, "Property not supported by this radar type").ThrowAsJavaScriptException();
        return env.Null();
    }
    PLUGIN_NAMESPACE::ControlInfo *ctrl = &dlg->m_ctrl[property_index];
    ControlInfoToNode(env, ctrl);
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
    Napi::Object result = Napi::Object::New(env);
    
    for (size_t i = 0; i < PLUGIN_NAMESPACE::CT_MAX; i++) {
        PLUGIN_NAMESPACE::RadarControlButton *button = dlg->m_button[i];
        if (button) {
         
            result.Set(control_type_identifiers[dlg->m_ctrl[i].type],
                       RadarControlInfoItemToNode(env, &dlg->m_ctrl[i], button->m_item));
       }
    }
    
    return result;
}

Napi::Value RadarInfoWrapper::GetGuardZones(const Napi::CallbackInfo& info) {
    PLUGIN_NAMESPACE::RadarInfo *ri = radar->m_radar[0];
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env, GUARD_ZONES);
    for (uint32_t i = 0; i < GUARD_ZONES; ++i) {
        Napi::Object def = Napi::Object::New(env);    
        def.Set("start_bearing", Napi::Number::New(env, ri->m_guard_zone[i]->m_start_bearing));
        def.Set("end_bearing", Napi::Number::New(env, ri->m_guard_zone[i]->m_start_bearing));
        def.Set("inner_range", Napi::Number::New(env, ri->m_guard_zone[i]->m_inner_range));
        def.Set("outer_range", Napi::Number::New(env, ri->m_guard_zone[i]->m_outer_range));
        def.Set("alarm_on", Napi::Boolean::New(env, ri->m_guard_zone[i]->m_alarm_on));
        def.Set("arpa_on", Napi::Boolean::New(env, ri->m_guard_zone[i]->m_arpa_on));
        def.Set("type", Napi::String::New(env, (ri->m_guard_zone[i]->m_type == PLUGIN_NAMESPACE::GZ_ARC) ? "GZ_ARC" : "GZ_CIRCLE"));
        result.Set(i, def);
    }
    return result;
}

Napi::Value RadarInfoWrapper::SetGuardZones(const Napi::CallbackInfo& info) {
    PLUGIN_NAMESPACE::RadarInfo *ri = radar->m_radar[0];
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsArray()) {
        Napi::TypeError::New(env, "Expected an array of guard zone definitions").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array inputArray = info[0].As<Napi::Array>();
    uint32_t length = inputArray.Length();

    for (uint32_t i = 0; (i < length) && (i < GUARD_ZONES); ++i) {        
        Napi::Value element = inputArray[i];
        if (!element.IsObject()) {
          Napi::TypeError::New(env, "Guard zone definitions must be objects").ThrowAsJavaScriptException();
          return env.Null();
        }

        Napi::Object guard_zone_def = element.As<Napi::Object>();
        ri->m_guard_zone[i]->m_start_bearing = (guard_zone_def.Has("start_bearing")
                                                ? guard_zone_def.Get("start_bearing").As<Napi::Number>().Int32Value()
                                                : 0);
        ri->m_guard_zone[i]->m_end_bearing = (guard_zone_def.Has("end_bearing")
                                              ? guard_zone_def.Get("end_bearing").As<Napi::Number>().Int32Value()
                                              : 0);
        ri->m_guard_zone[i]->m_inner_range = (guard_zone_def.Has("inner_range")
                                              ? guard_zone_def.Get("inner_range").As<Napi::Number>().Int32Value()
                                              : 0);
        ri->m_guard_zone[i]->m_outer_range = (guard_zone_def.Has("outer_range")
                                              ? guard_zone_def.Get("outer_range").As<Napi::Number>().Int32Value()
                                              : 0);
        ri->m_guard_zone[i]->m_alarm_on = (guard_zone_def.Has("alarm_on")
                                           ? guard_zone_def.Get("alarm_on").As<Napi::Boolean>().Value()
                                           : 0);
        ri->m_guard_zone[i]->m_arpa_on = (guard_zone_def.Has("arpa_on")
                                           ? guard_zone_def.Get("arpa_on").As<Napi::Boolean>().Value()
                                           : 0);

        ri->m_guard_zone[i]->m_type = PLUGIN_NAMESPACE::GZ_CIRCLE;
        if (guard_zone_def.Has("type") && (guard_zone_def.Get("type").As<Napi::String>().Utf8Value() == "GZ_ARC")) {
            ri->m_guard_zone[i]->m_type = PLUGIN_NAMESPACE::GZ_ARC;
        }
    }
    return env.Null();
}

Napi::Value RadarInfoWrapper::GetSettings(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    radar->SaveConfig();
    return settings->DumpToJson(env);
}

Napi::Value RadarInfoWrapper::SetSettings(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected settings object").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object args = info[0].As<Napi::Object>();
    settings->LoadFromJson(args);
    radar->LoadConfig();
}

Napi::Value RadarInfoWrapper::SetPosition(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected coordinate object {lat, lon}").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object args = info[0].As<Napi::Object>();
    if (!args.Has("lat")) {
       radar->m_bpos_set = false;
       return env.Null();
    }
    
    radar->m_ownship.lat = args.Get("lat").As<Napi::Number>().FloatValue();
    radar->m_ownship.lon = args.Get("lon").As<Napi::Number>().FloatValue();
    radar->m_bpos_set = true;
    
    for (size_t r = 0; r < radar->m_settings.radar_count; r++) {
      wxCriticalSectionLocker lock(radar->m_radar[r]->m_exclusive);
      if (radar->m_radar[r]) {
        radar->m_radar[r]->SetRadarPosition(radar->m_ownship, radar->m_hdt);
      }
    }
    return env.Null();
}

Napi::Function RadarInfoWrapper::GetClass(Napi::Env env) {
    return DefineClass(env, "RadarInfo", {
      //InstanceMethod("init", &RadarInfoWrapper::Init),
      InstanceMethod("shutdown", &RadarInfoWrapper::Shutdown),
      InstanceMethod("setTransmit", &RadarInfoWrapper::SetTransmit),
      InstanceMethod("getType", &RadarInfoWrapper::GetType),
      InstanceMethod("setProperty", &RadarInfoWrapper::SetProperty),
      InstanceMethod("getProperty", &RadarInfoWrapper::GetProperty),
      InstanceMethod("getPropertyType", &RadarInfoWrapper::GetPropertyType),
      InstanceMethod("getProperties", &RadarInfoWrapper::GetProperties),
      InstanceMethod("getGuardZones", &RadarInfoWrapper::GetGuardZones),
      InstanceMethod("setGuardZones", &RadarInfoWrapper::SetGuardZones),
      InstanceMethod("getSettings", &RadarInfoWrapper::GetSettings),
      InstanceMethod("setSettings", &RadarInfoWrapper::SetSettings),
      InstanceMethod("setPosition", &RadarInfoWrapper::SetPosition)
    });
}
