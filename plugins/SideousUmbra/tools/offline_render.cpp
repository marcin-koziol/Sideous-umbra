// throwaway dev aid, not wired into CMakeLists.txt - see docs comment in the
// task spec. Compile ad hoc:
//   g++ $(pkg-config --cflags --libs cairo) -std=c++17 -o /tmp/render offline_render.cpp
// then run it and inspect the PNG it writes.
#include "../ui/UIPainter.hpp"

#include <cstdio>

int main(int argc, char** argv)
{
    const float width = 640.0f;
    sideous::ui::Layout layout = sideous::ui::buildLayout(width);

    std::printf("layout width=%.1f height=%.1f\n", layout.width, layout.height);

    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, (int)layout.width, (int)std::ceil(layout.height));
    cairo_t* cr = cairo_create(surface);

    float values[sideous::kParamCount];
    for (uint32_t i = 0; i < sideous::kParamCount; ++i)
        values[i] = sideous::getParamInfo(i).def;

    sideous::ui::PaintState state;
    state.values = values;

    sideous::ui::paint(cr, layout, state);

    const char* outPath = argc > 1 ? argv[1] : "/tmp/sideous_umbra_render.png";
    cairo_surface_write_to_png(surface, outPath);
    std::printf("wrote %s\n", outPath);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return 0;
}
