#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/desktop/Workspace.hpp"
#include "hyprland/src/desktop/view/Window.hpp"
#include "hyprland/src/managers/fullscreen/FullscreenController.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"

// using namespace Desktop::View;
using namespace Fullscreen;

#ifdef DEBUG
#include <string>
#endif
#include <unordered_map>

struct Status {
  PHLWINDOW window;
  SFullscreenMode state;
};

std::unordered_map<PHLWINDOW, Status> previous_fs{};

inline HANDLE PHANDLE = nullptr;

// inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;
inline CFunctionHook *g_setFullscreenModeHook = nullptr;

typedef void (*origSetFullscreenMode)(CFullscreenController *, PHLWINDOW,
                                      std::optional<eFullscreenMode>,
                                      std::optional<eFullscreenMode>,
                                      std::optional<bool>);

void hkSetFullscreenMode(CFullscreenController *thisptr, PHLWINDOW pWindow,
                         std::optional<eFullscreenMode> internal = std::nullopt,
                         std::optional<eFullscreenMode> client = std::nullopt,
                         std::optional<bool> layoutAware = std::nullopt) {

#ifdef DEBUG
  HyprlandAPI::addNotification(
      PHANDLE,
      pWindow->m_title + ";" + std::to_string(pWindow->isFullscreen()) + "->" +
          std::to_string(state.internal) + ";" + std::to_string(state.client) +
          "~~" + std::to_string(pWindow->m_workspace->m_id) + "(" +
          std::to_string(pWindow->m_workspace->m_hasFullscreenWindow) + ")",
      CHyprColor(1, 1, 1, 1), 5000);
#endif

  SFullscreenMode modes = thisptr->getFullscreenModes(pWindow);

  if ((!internal || modes.internal == *internal) &&
      (!client || modes.client == *client))
    return;

  eFullscreenMode target_internal = (internal) ? *internal : modes.internal;
  eFullscreenMode target_client = (client) ? *client : modes.client;

  if (/*pWindow->m_bPinned && */ !thisptr->isFullscreen(pWindow) &&
      target_internal != FSMODE_NONE) {
    auto w = pWindow->m_workspace;
    if (thisptr->hasFullscreen(w)) {
      PHLWINDOW wfs = thisptr->getFullscreenWindow(w);
      previous_fs[pWindow] = {wfs, thisptr->getFullscreenModes(wfs)};
    }
  }

  (*(origSetFullscreenMode)g_setFullscreenModeHook->m_original)(
      thisptr, pWindow, internal, client, layoutAware);

  if (pWindow->m_pinFullscreened && target_internal == FSMODE_NONE) {
    pWindow->m_pinned = true;
    pWindow->m_pinFullscreened = false;
  }

  if (target_internal == FSMODE_NONE && previous_fs.contains(pWindow)) {
    Status old_fs = previous_fs[pWindow];
    if (valid(old_fs.window))
      (*(origSetFullscreenMode)g_setFullscreenModeHook->m_original)(
          thisptr, old_fs.window, old_fs.state.internal, old_fs.state.client,
          std::nullopt);

    previous_fs.erase(pWindow);
  }
}

inline CFunctionHook *g_pCloseWindowHook = nullptr;

typedef void (*origCloseWindow)(CCompositor *, PHLWINDOW);

void hkCloseWindow(CCompositor *thisptr, PHLWINDOW pWindow) {
  HyprlandAPI::addNotification(PHANDLE, "", CHyprColor(1, 1, 1, 1), 0);
  if (Fullscreen::controller()->isFullscreen(pWindow) &&
      previous_fs.contains(pWindow)) {
    Status old_fs = previous_fs[pWindow];
    if (valid(old_fs.window))
      (*(origSetFullscreenMode)g_setFullscreenModeHook->m_original)(
          &*Fullscreen::controller(), old_fs.window, old_fs.state.internal,
          old_fs.state.client, std::nullopt);

    previous_fs.erase(pWindow);
  }
  (*(origCloseWindow)g_pCloseWindowHook->m_original)(thisptr, pWindow);
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  static const auto SWFS_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "setFullscreenMode");
  g_setFullscreenModeHook = HyprlandAPI::createFunctionHook(
      handle, SWFS_METHODS[0].address, (void *)&hkSetFullscreenMode);
  g_setFullscreenModeHook->hook();

  static const auto CW_METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "closeWindow");
  g_pCloseWindowHook = HyprlandAPI::createFunctionHook(
      handle, CW_METHODS[0].address, (void *)&hkCloseWindow);
  g_pCloseWindowHook->hook();

  return {"hypr_fullscreen_plus", "Makes fullscreen better", "louisbui63",
          "0.0.4"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
