build:
	g++ -shared -fPIC --no-gnu-unique main.cpp -o hypr_fullscreen_plus.so -g `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++2b -O2 -DWLR_USE_UNSTABLE
