#include "hyprland/src/Compositor.hpp"
#include "hyprland/src/Window.hpp"
#include "hyprland/src/plugins/PluginAPI.hpp"
#include <cstdint>
#include <unordered_map>

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;

struct {
  bool pinned;
  Vector2D size;
  Vector2D position;
  bool was_already_fs;
  CWindow *old_fs;

} typedef Status;

std::unordered_map<uintptr_t, Status> PINNED_FULLSCREEN{};

typedef void (*origSetWindowFullscreen)(CCompositor *, CWindow *, bool, int8_t);

void hkSetWindowFullscreen(CCompositor *thisptr, CWindow *pWindow, bool on,
                           int8_t mode) {
  const auto PWORKSPACE = thisptr->getWorkspaceByID(pWindow->m_iWorkspaceID);
  if (on) {
    bool was_already_fs = false;
    CWindow *old_fs = nullptr;
    if (PWORKSPACE->m_bHasFullscreenWindow) {
      CWindow *fs_w =
          thisptr->getFullscreenWindowOnWorkspace(pWindow->m_iWorkspaceID);
      thisptr->setWindowFullscreen(fs_w, false, FULLSCREEN_FULL);
      was_already_fs = true;
      old_fs = fs_w;
    }

    Status st = {pWindow->m_bPinned, pWindow->m_vRealSize.vec(),
                 pWindow->m_vRealPosition.vec(), was_already_fs, old_fs};
    pWindow->m_bPinned = false;

    PINNED_FULLSCREEN[(uintptr_t)pWindow] = st;
    pWindow->m_bPinned = false;
  }

  (*(origSetWindowFullscreen)g_pSetWindowFullscreenHook->m_pOriginal)(
      thisptr, pWindow, on, mode);
  if (!on && PINNED_FULLSCREEN.contains((uintptr_t)pWindow)) {
    auto u = PINNED_FULLSCREEN[(uintptr_t)pWindow];
    pWindow->m_bPinned = u.pinned;
    if (pWindow->m_bIsFloating) {
      pWindow->m_vRealSize = u.size;
      pWindow->m_vRealPosition = u.position;
    }
    if (u.was_already_fs) {
      thisptr->setWindowFullscreen(u.old_fs, true, FULLSCREEN_FULL);
    }
    PINNED_FULLSCREEN.erase((uintptr_t)pWindow);
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
