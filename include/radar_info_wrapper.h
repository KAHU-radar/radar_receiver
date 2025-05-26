#ifndef RADAR_INFO_WRAPPER_H
#define RADAR_INFO_WRAPPER_H

#include <napi.h>
#include "radar_pi.h"
#include "RadarInfo.h"
#include "RadarControlItem.h"
#include "settings.h"

class TimedUpdateThread : public wxThread {
public:
    TimedUpdateThread(PLUGIN_NAMESPACE::radar_pi* pi);
    ~TimedUpdateThread();
    void Shutdown(void);
    volatile bool m_is_shutdown;
protected:
    void* Entry(void);
private:
    PLUGIN_NAMESPACE::radar_pi* m_pi;
    volatile bool m_shutdown;
    wxCriticalSection m_exclusive;
};


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
    Napi::Value GetGuardZones(const Napi::CallbackInfo& info);
    Napi::Value SetGuardZones(const Napi::CallbackInfo& info);
    Napi::Value GetSettings(const Napi::CallbackInfo& info);
    Napi::Value SetSettings(const Napi::CallbackInfo& info);

private:
    PLUGIN_NAMESPACE::radar_pi* radar;
    Settings* settings;
    TimedUpdateThread* timed_update_thread;
    Napi::ThreadSafeFunction process_radar_spoke_fn;
    Napi::ThreadSafeFunction notify_fn;
};

#endif
