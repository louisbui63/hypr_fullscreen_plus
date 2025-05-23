#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/desktop/Window.hpp"
#include "hyprland/src/desktop/Workspace.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"

#ifdef DEBUG
#include <string>
#endif
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

#ifdef DEBUG
  HyprlandAPI::addNotification(
      PHANDLE,
      pWindow->m_title + ";" + std::to_string(pWindow->isFullscreen()) + "->" +
          std::to_string(state.internal) + ";" + std::to_string(state.client) +
          "~~" + std::to_string(pWindow->m_workspace->m_id) + "(" +
          std::to_string(pWindow->m_workspace->m_hasFullscreenWindow) + ")",
      CHyprColor(1, 1, 1, 1), 5000);
#endif

  if (pWindow->m_fullscreenState.internal == state.internal &&
      pWindow->m_fullscreenState.client == state.client)
    return;

  if (/*pWindow->m_bPinned && */ !pWindow->isFullscreen() &&
      state.internal != FSMODE_NONE) {
    auto w = pWindow->m_workspace;
    if (w->m_hasFullscreenWindow) {
      PHLWINDOW wfs = w->getFullscreenWindow();
      previous_fs[pWindow] = {wfs, wfs->m_fullscreenState};
    }
  }

  (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_original)(
      thisptr, pWindow, state);

  if (pWindow->m_pinFullscreened && state.internal == FSMODE_NONE) {
    pWindow->m_pinned = true;
    pWindow->m_pinFullscreened = false;
  }

  if (state.internal == FSMODE_NONE && previous_fs.contains(pWindow)) {
    Status old_fs = previous_fs[pWindow];
    if (valid(old_fs.window))
      (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_original)(
          thisptr, old_fs.window, old_fs.state);

    previous_fs.erase(pWindow);
  }
}

inline CFunctionHook *g_pCloseWindowHook = nullptr;

typedef void (*origCloseWindow)(CCompositor *, PHLWINDOW);

void hkCloseWindow(CCompositor *thisptr, PHLWINDOW pWindow) {
  if (pWindow->isFullscreen() && previous_fs.contains(pWindow)) {
    Status old_fs = previous_fs[pWindow];
    if (valid(old_fs.window))
      (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_original)(
          thisptr, old_fs.window, old_fs.state);

    previous_fs.erase(pWindow);
  }

  (*(origCloseWindow)g_pCloseWindowHook->m_original)(thisptr, pWindow);
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  static const auto SWFS_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "setWindowFullscreenState");
  g_pSetWindowFullscreenHook = HyprlandAPI::createFunctionHook(
      handle, SWFS_METHODS[0].address, (void *)&hkSetWindowFullscreen);
  g_pSetWindowFullscreenHook->hook();

  static const auto CW_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "closeWindow");
  g_pCloseWindowHook = HyprlandAPI::createFunctionHook(
      handle, CW_METHODS[0].address, (void *)&hkCloseWindow);
  g_pCloseWindowHook->hook();

  return {"hypr_fullscreen_plus", "Makes fullscreen better", "louisbui63",
          "0.0.3"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
