#include "settings.h"


void Settings::LoadFromJson(const Napi::Object& jsObject) {
    config->DeleteAll();
    WriteSectionRecursive(config, jsObject, "");
}


Napi::Object Settings::DumpToJson(Napi::Env env) {
    Napi::Object root = Napi::Object::New(env);
    ReadSectionRecursive(config, root, "", env);
    return root;
}

void Settings::WriteSectionRecursive(wxConfigBase* config, const Napi::Object& jsObject, const wxString& path) {
    Napi::Array propNames = jsObject.GetPropertyNames();
    for (uint32_t i = 0; i < propNames.Length(); ++i) {
        std::string key = propNames.Get(i).As<Napi::String>().Utf8Value();
        Napi::Value val = jsObject.Get(key);

        wxString fullPath = path + "/" + wxString::FromUTF8(key.c_str());

        if (val.IsObject()) {
            config->SetPath(path);
            config->SetPath(key);
            WriteSectionRecursive(config, val.As<Napi::Object>(), fullPath);
            config->SetPath("..");
        } else if (val.IsNumber()) {
            config->Write(fullPath, val.As<Napi::Number>().DoubleValue());
        } else if (val.IsBoolean()) {
            config->Write(fullPath, val.As<Napi::Boolean>().Value());
        } else if (val.IsString()) {
            config->Write(fullPath, wxString::FromUTF8(val.As<Napi::String>().Utf8Value()));
        }
    }
}

void Settings::ReadSectionRecursive(wxConfigBase* config, Napi::Object& jsObject, const wxString& path, Napi::Env env) {
    long idx;
    wxString name, value;

    config->SetPath(path);

    // Read entries (keys)
    bool cont = config->GetFirstEntry(name, idx);
    while (cont) {
        config->Read(name, &value);
        jsObject.Set(name.ToUTF8().data(), Napi::String::New(env, value.ToUTF8().data()));
        cont = config->GetNextEntry(name, idx);
    }

    // Recurse into groups (subsections)
    cont = config->GetFirstGroup(name, idx);
    while (cont) {
        Napi::Object subObj = Napi::Object::New(env);
        ReadSectionRecursive(config, subObj, name, env);
        jsObject.Set(name.ToUTF8().data(), subObj);
        cont = config->GetNextGroup(name, idx);
    }

    config->SetPath("..");
}
