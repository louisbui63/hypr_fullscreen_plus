#include "Hyprland/src/Window.hpp"
#include "Hyprland/src/plugins/PluginAPI.hpp"
#include <cstdint>
#include <unordered_map>

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook *g_pSetWindowFullscreenHook = nullptr;

struct {
  bool pinned;
  Vector2D size;
  Vector2D position;

} typedef Status;

std::unordered_map<uintptr_t, Status> PINNED_FULLSCREEN{};

typedef void (*origSetWindowFullscreen)(void *, CWindow *, bool, int8_t);

void hkSetWindowFullscreen(void *thisptr, CWindow *pWindow, bool on,
                           int8_t mode) {
  if (on) {

    Status st = {pWindow->m_bPinned, pWindow->m_vRealSize.vec(),
                 pWindow->m_vRealPosition.vec()};
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
