{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cpp",
        "src/radar_info_wrapper.cpp",
      ],
      "include_dirs": [
        "include",
        "<!(node -p \"require('node-addon-api').include\")",
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "deps/wxWidgets/dist/include/wx-3.3",
        "deps/wxWidgets/dist/lib/wx/include/gtk3-unicode-static-3.3",
        "deps/wxWidgets/dist/lib"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc": [
        "<!(deps/wxWidgets/dist/bin/wx-config --cxxflags)",
        "-std=c++17",
        "-frtti",
        "-fexceptions"],
      "defines": ["NAPI_CPP_EXCEPTIONS"],
      "libraries": [
        "-Wl,--whole-archive",
        "<!(deps/wxWidgets/dist/bin/wx-config --libs --static)",
        "-Wl,--no-whole-archive",
        "-lX11", "-lXxf86vm", "-lGL", "-lGLU", "-lpthread", "-ldl", "-lm"
      ]
    }
  ]
}
