#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/desktop/Window.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;

struct {
  bool pinned;
  Vector2D size;
  Vector2D position;
  bool was_already_fs;
  PHLWINDOW old_fs;

} typedef Status;

std::unordered_map<PHLWINDOW, Status> PINNED_FULLSCREEN{};

typedef void (*origSetWindowFullscreen)(CCompositor *, PHLWINDOW, bool, int8_t);

void hkSetWindowFullscreen(CCompositor *thisptr, PHLWINDOW pWindow, bool on,
                           int8_t mode) {

  const auto PWORKSPACE = thisptr->getWorkspaceByID(pWindow->workspaceID());

  if (on && pWindow->m_bIsFloating) {
    bool was_already_fs = false;
    PHLWINDOW old_fs = nullptr;
    if (PWORKSPACE->m_bHasFullscreenWindow) {
      PHLWINDOW fs_w =
          thisptr->getFullscreenWindowOnWorkspace(pWindow->workspaceID());

      thisptr->setWindowFullscreen(fs_w, false, FULLSCREEN_FULL);
      was_already_fs = true;
      old_fs = fs_w;
    }

    Status st = {pWindow->m_bPinned, pWindow->m_vRealSize.value(),
                 pWindow->m_vRealPosition.value(), was_already_fs, old_fs};

    PINNED_FULLSCREEN[pWindow] = st;
    if (pWindow->m_bPinned) {
      pWindow->m_bPinned = false;
      // pWindow->workspaceID() =
      //     thisptr->getMonitorFromID(pWindow->m_iMonitorID)->activeWorkspace;
      pWindow->moveToWorkspace(
          thisptr->getMonitorFromID(pWindow->m_iMonitorID)->activeWorkspace);

      pWindow->updateDynamicRules();
      thisptr->updateWindowAnimatedDecorationValues(pWindow);

      const auto PWORKSPACE = thisptr->getWorkspaceByID(pWindow->workspaceID());

      PWORKSPACE->m_pLastFocusedWindow = thisptr->vectorToWindowUnified(
          g_pInputManager->getMouseCoordsInternal(), USE_PROP_TILED);
    }
    pWindow->m_bIsFloating = !pWindow->m_bIsFloating;
    pWindow->updateDynamicRules();
    g_pLayoutManager->getCurrentLayout()->changeWindowFloatingMode(pWindow);
  }

  (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
      thisptr, pWindow, on, mode);

  if (!on && PINNED_FULLSCREEN.contains(pWindow)) {
    pWindow->m_bIsFloating = !pWindow->m_bIsFloating;
    pWindow->updateDynamicRules();
    g_pLayoutManager->getCurrentLayout()->changeWindowFloatingMode(pWindow);
    auto u = PINNED_FULLSCREEN[pWindow];
    if (pWindow->m_bIsFloating) {
      // pWindow->m_vRealSize = u.size;
      g_pLayoutManager->getCurrentLayout()->resizeActiveWindow(
          u.size - pWindow->m_vRealSize.goal(), CORNER_NONE, pWindow);
      if (pWindow->m_vRealSize.goal().x > 1 &&
          pWindow->m_vRealSize.goal().y > 1)
        pWindow->setHidden(false);
      // pWindow->m_vRealPosition = u.position;
      g_pLayoutManager->getCurrentLayout()->moveActiveWindow(
          u.position - pWindow->m_vRealPosition.goal(), pWindow);
    }

    if (pWindow->m_bIsFloating && u.pinned) {
      pWindow->m_bPinned = true;
      // pWindow->workspaceID() =
      //     thisptr->getMonitorFromID(pWindow->m_iMonitorID)->activeWorkspace;
      pWindow->moveToWorkspace(
          thisptr->getMonitorFromID(pWindow->m_iMonitorID)->activeWorkspace);

      pWindow->updateDynamicRules();
      thisptr->updateWindowAnimatedDecorationValues(pWindow);

      const auto PWORKSPACE = thisptr->getWorkspaceByID(pWindow->workspaceID());

      PWORKSPACE->m_pLastFocusedWindow = thisptr->vectorToWindowUnified(
          g_pInputManager->getMouseCoordsInternal(), USE_PROP_TILED);
    }

    if (u.was_already_fs) {
      if (valid(u.old_fs))
        (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
            thisptr, u.old_fs, true, FULLSCREEN_FULL);
    }

    PINNED_FULLSCREEN.erase(pWindow);
  }
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  static const auto METHODS =
      HyprlandAPI::findFunctionsByName(PHANDLE, "setWindowFullscreen");
  g_pSetWindowFullscreenHook = HyprlandAPI::createFunctionHook(
      handle, METHODS[0].address, (void *)&hkSetWindowFullscreen);

  g_pSetWindowFullscreenHook->hook();

  return {"hypr_fullscreen_plus", "Makes fullscreen better", "louisbui63",
          "0.0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
