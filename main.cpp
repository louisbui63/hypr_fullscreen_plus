#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/desktop/Window.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;

typedef void (*origSetWindowFullscreen)(CCompositor *, PHLWINDOW,
                                        SFullscreenState);

void hkSetWindowFullscreen(CCompositor *thisptr, PHLWINDOW pWindow,
                           SFullscreenState state) {

  (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
      thisptr, pWindow, state);

  if (pWindow->m_bPinFullscreened && state.internal == FSMODE_NONE) {
    pWindow->m_bPinned = true;
    pWindow->m_bPinFullscreened = false;
  }
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  static const auto METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "setWindowFullscreenState");
  g_pSetWindowFullscreenHook = HyprlandAPI::createFunctionHook(
      handle, METHODS[0].address, (void *)&hkSetWindowFullscreen);

  g_pSetWindowFullscreenHook->hook();

  return {"hypr_fullscreen_plus", "Makes fullscreen better", "louisbui63",
          "0.0.2"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
