#pragma once

namespace horizon {
void preferences_provider_attach_canvas(class CanvasGL *ca, bool layer);
void preferences_apply_to_canvas(class CanvasGL *ca, const class Preferences &prefs);
} // namespace horizon
