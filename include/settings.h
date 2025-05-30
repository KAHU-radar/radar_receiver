#include <wx/fileconf.h>
#include <wx/mstream.h>
#include <napi.h>

class Settings {
public:
    Settings() {
        config = new wxFileConfig(wxEmptyString, wxEmptyString, wxEmptyString, wxEmptyString,
                                  wxCONFIG_USE_LOCAL_FILE);
    }

    ~Settings() {
        delete config;
    }

    void LoadFromJson(const Napi::Object& jsObject);
    Napi::Object DumpToJson(Napi::Env env);

    wxFileConfig* config;

private:
    void WriteSectionRecursive(wxConfigBase* config, const Napi::Object& jsObject, const wxString& path);
    void ReadSectionRecursive(wxConfigBase* config, Napi::Object& jsObject, const wxString& path, Napi::Env env);
};
