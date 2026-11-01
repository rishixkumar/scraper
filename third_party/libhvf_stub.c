/* Homebrew FreeType 2.15 links /usr/lib/libhvf.dylib.
 * That library is absent on macOS 15.3. These stubs exist so dyld can load
 * FreeType. They report failure, and FreeType falls back for ordinary fonts.
 * Point DYLD_FALLBACK_LIBRARY_PATH at the directory that contains the built
 * libhvf.dylib. See the README.
 */
#include <stddef.h>

__attribute__((visibility("default"))) size_t HVF_part_renderer_storage_size(void) {
  return 0;
}

__attribute__((visibility("default"))) int HVF_open_part_renderer(
    const void* hvgl, size_t hvgl_size, const void* hvpm, size_t hvpm_size,
    void* storage, size_t storage_size) {
  (void)hvgl;
  (void)hvgl_size;
  (void)hvpm;
  (void)hvpm_size;
  (void)storage;
  (void)storage_size;
  return 1;
}

__attribute__((visibility("default"))) void HVF_close_part_renderer(void* renderer) {
  (void)renderer;
}

__attribute__((visibility("default"))) int HVF_render_part_axis_count(void* renderer) {
  (void)renderer;
  return 0;
}

__attribute__((visibility("default"))) int HVF_set_axis_value(void* renderer, int axis,
                                                            double value) {
  (void)renderer;
  (void)axis;
  (void)value;
  return 1;
}

__attribute__((visibility("default"))) int HVF_set_render_part(void* renderer,
                                                             unsigned int part) {
  (void)renderer;
  (void)part;
  return 1;
}

__attribute__((visibility("default"))) int HVF_render_current_part(void* renderer,
                                                                 void* callback,
                                                                 void* context) {
  (void)renderer;
  (void)callback;
  (void)context;
  return 1;
}

__attribute__((visibility("default"))) void HVF_clear_part_cache(void* renderer) {
  (void)renderer;
}
