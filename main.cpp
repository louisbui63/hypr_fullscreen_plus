#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/desktop/Window.hpp"
#include "hyprland/src/desktop/Workspace.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"

#include <unordered_map>

struct Status {
  PHLWINDOW window;
  SFullscreenState state;
};

std::unordered_map<PHLWINDOW, Status> previous_fs{};

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;

typedef void (*origSetWindowFullscreen)(CCompositor *, PHLWINDOW,
                                        SFullscreenState);

void hkSetWindowFullscreen(CCompositor *thisptr, PHLWINDOW pWindow,
                           SFullscreenState state) {

  if (pWindow->m_sFullscreenState.internal == state.internal &&
      pWindow->m_sFullscreenState.client == state.client)
    return;

  if (pWindow->m_bPinned && !pWindow->isFullscreen() &&
      state.internal != FSMODE_NONE) {
    auto w = pWindow->m_pWorkspace;
    if (w->m_bHasFullscreenWindow) {
      PHLWINDOW wfs = w->getFullscreenWindow();
      previous_fs[pWindow] = {wfs, wfs->m_sFullscreenState};
    }
  }

  (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
      thisptr, pWindow, state);

  if (pWindow->m_bPinFullscreened && state.internal == FSMODE_NONE) {
    pWindow->m_bPinned = true;
    pWindow->m_bPinFullscreened = false;
  }

  if (state.internal == FSMODE_NONE && previous_fs.contains(pWindow)) {
    Status old_fs = previous_fs[pWindow];
    if (valid(old_fs.window))
      (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
          thisptr, old_fs.window, old_fs.state);

    previous_fs.erase(pWindow);
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
