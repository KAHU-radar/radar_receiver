{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cpp",
        "src/radar_info_wrapper.cpp",
        "src/settings.cpp",

        "src/radar_pi.cpp",
        "src/RadarInfo.cpp",
        "src/ControlsDialog.cpp",
        "src/Arpa.cpp",

        "src/GuardZone.cpp",
        "src/Kalman.cpp",
        "src/RadarFactory.cpp",
        "src/socketutil.cpp",

        "src/emulator/EmulatorControl.cpp",
        "src/emulator/EmulatorControlsDialog.cpp",
        "src/emulator/EmulatorReceive.cpp",
        "src/garminhd/GarminHDControl.cpp",
        "src/garminhd/GarminHDControlsDialog.cpp",
        "src/garminhd/GarminHDReceive.cpp",
        "src/garminxhd/GarminxHDControl.cpp",
        "src/garminxhd/GarminxHDControlsDialog.cpp",
        "src/garminxhd/GarminxHDReceive.cpp",
        "src/navico/NavicoControl.cpp",
        "src/navico/NavicoControlsDialog.cpp",
        "src/navico/NavicoLocate.cpp",
        "src/navico/NavicoReceive.cpp",
        "src/raymarine/RaymarineLocate.cpp",
        "src/raymarine/RaymarineReceive.cpp",
        "src/raymarine/RME120Control.cpp",
        "src/raymarine/RME120ControlsDialog.cpp",
        "src/raymarine/RMQuantumControl.cpp",
        "src/raymarine/RMQuantumControlsDialog.cpp",
      ],
      "include_dirs": [
        "include",

        "include/emulator",
        "include/garminhd",
        "include/garminxhd",
        "include/navico",
        "include/raymarine",
        
        "<!(node -p \"require('node-addon-api').include\")",
        "<!(node -p \"require('node-addon-api').include_dir\")",
        
        "deps/wxWidgets/build-static/dist/include/wx-3.2",
        "deps/wxWidgets/build-static/dist/lib/wx/include/gtk3-unicode-static-3.2"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc": [
        "-g", "-O0",
        "-D_FILE_OFFSET_BITS=64",
        "-D__WXGTK__",
        "-std=c++17",
        "-frtti",
        "-fexceptions",
        "-pthread"],
      "defines": ["NAPI_CPP_EXCEPTIONS", "WXWIN_COMPATIBILITY_2_8=1", "wxUSE_UNSAFE_WXSTRING_CONV=1"],
      "libraries": [
        "-Wl,--whole-archive",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu_net-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu_xml-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxexpat-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxregexu-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxzlib-3.2",
        "-Wl,--no-whole-archive",
        "-lpthread", "-ldl", "-lm"
      ]
    }
  ]
}
