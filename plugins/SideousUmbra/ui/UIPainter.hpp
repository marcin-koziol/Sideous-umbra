/*
 * Sideous Umbra - GUI layout and Cairo painting, shared between the real
 * plugin UI and an offline PNG renderer used to check the look during
 * development (same technique as sideous-noise, see its UIPainter.hpp).
 *
 * Light chassis (near-white, close to sideous-noise's own pastel theme) with
 * Umbra the cat painted in her own original umbra.svg colors (see
 * paintUmbra()) rather than a flat tint - keeps her eyes' brightening glow
 * (paintUmbraEyes(), driven by the live output level) reading clearly against
 * a dark silhouette instead of getting lost in a same-brightness fill. Pride
 * stays as the accent language elsewhere: panel titles cycle through five of
 * the six Pride flag colors (red/orange/yellow/green/blue), left to right,
 * top to bottom, in flag order. Smaller widget vocabulary than sideous-noise's
 * UIPainter.hpp - no selector/dropdown widgets, since Sideous Umbra has no
 * enum parameters - but it does have the same file-based preset bar (see
 * ui/PresetStore.hpp, ui/FactoryPresets.hpp, Button/PresetBarLayout/
 * paintPresetBar() below), ported from sideous-noise's UIPainter.hpp.
 */

#pragma once

#include <cairo.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>
#include <algorithm>

#include "../Params.hpp"
#include "UmbraCatImage.hpp"
#include "UmbraEyesImage.hpp"

namespace sideous {
namespace ui {

// -----------------------------------------------------------------------------------------------------------
// palette - light chassis, Pride accents

struct Color { double r, g, b; };

static constexpr Color kBg        { 0.973, 0.973, 0.980 }; // near-white
static constexpr Color kPanelBg   { 0.925, 0.925, 0.937 }; // soft-light panel fill
static constexpr Color kPanelEdge { 0.780, 0.780, 0.808 }; // subtle grey-purple outline
static constexpr Color kTextMain  { 0.106, 0.106, 0.129 }; // near-black
static constexpr Color kTextDim   { 0.451, 0.451, 0.494 }; // muted grey, secondary text
static constexpr Color kKnobBody  { 0.957, 0.957, 0.965 }; // knob face, a touch darker than bg
static constexpr Color kKnobRing  { 0.855, 0.855, 0.875 }; // knob track (unfilled arc)
static constexpr Color kBtnBg     { 0.988, 0.988, 0.992 }; // unselected preset-bar button fill, a touch lighter than kBg

// standard Pride flag hex, in flag order
static constexpr Color kPrideRed    { 0.894, 0.012, 0.012 }; // E40303
static constexpr Color kPrideOrange { 1.000, 0.549, 0.000 }; // FF8C00
static constexpr Color kPrideYellow { 1.000, 0.929, 0.000 }; // FFED00
static constexpr Color kPrideGreen  { 0.000, 0.502, 0.149 }; // 008026
static constexpr Color kPrideBlue   { 0.000, 0.302, 1.000 }; // 004DFF
static constexpr Color kPridePurple { 0.451, 0.161, 0.510 }; // 732982

// destructive accent (preset Delete) - a muted terracotta, deliberately
// softer/duller than the vivid kPrideRed used for the Voice panel so
// "destructive action" doesn't visually compete with the Pride accent language
static constexpr Color kAccentCoral { 0.867, 0.408, 0.400 };

inline void setColor(cairo_t* cr, Color c, double a = 1.0) noexcept
{
    cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

// panel accents double as both graphical elements (knob rings, fills - fine
// at full brightness) and panel title text - but on this light chassis, a
// high-luminance accent like Pride yellow is nearly illegible as text on the
// near-white panel fill. Darken (toward black, same hue) any accent whose
// perceptual luminance is too high to read as text; low-luminance accents
// (red/green/blue/purple) already pass through unchanged.
inline Color titleTextColor(Color c) noexcept
{
    const double luma = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    if (luma < 0.6)
        return c;
    const double darken = 0.55;
    return { c.r * darken, c.g * darken, c.b * darken };
}

// -----------------------------------------------------------------------------------------------------------
// layout model

struct Knob
{
    uint32_t param;
    float cx, cy, radius;
    const char* label;
    Color accent;
};

struct PanelBox { float x, y, w, h; const char* title; Color accent; };

struct EnvelopeGraph
{
    uint32_t attackParam, decayParam, sustainParam, releaseParam, curveParam;
    float x, y, w, h;
    Color accent;
};

struct CatBox { float x, y, w, h; };

// a plain clickable button, not tied to a DPF parameter - presets are pure
// UI-side state (see ui/PresetStore.hpp), so Prev/Next/Save/Delete can't
// reuse the param-driven Knob widget the rest of the UI is built from.
struct Button { float x, y, w, h; const char* label; Color accent; };

struct PresetBarLayout
{
    Button prev, next, save, del;
    float nameX, nameY, nameW, nameH;
    Color accent;
};

struct Layout
{
    float width, height;
    CatBox cat;
    std::vector<PanelBox> panels;
    std::vector<Knob> knobs;
    std::vector<EnvelopeGraph> envelopeGraphs;
    PresetBarLayout presetBar;
};

// -----------------------------------------------------------------------------------------------------------
// layout builder

inline void addKnobRow(std::vector<Knob>& knobs, float panelX, float panelW, float rowY, float radius,
                        Color accent, std::initializer_list<std::pair<uint32_t, const char*>> items)
{
    const float slot = panelW / (float)items.size();
    size_t i = 0;
    for (const auto& item : items)
    {
        const float cx = panelX + slot * ((float)i + 0.5f);
        knobs.push_back({ item.first, cx, rowY, radius, item.second, accent });
        ++i;
    }
}

// vertical space a single-row knob panel needs, given the knob radius:
// 26px title zone + 8px gap + knob diameter + 14px label space + 10px pad
inline float knobPanelHeight(float radius) noexcept { return 58.0f + radius * 2.0f; }
inline float knobRowCenterY(float panelY, float radius) noexcept { return panelY + 34.0f + radius; }

inline Layout buildLayout(float width)
{
    Layout L;
    L.width = width;

    const float margin = 12.0f;
    const float gap = 8.0f;

    // --- header: title/subtitle (no separator line - redundant this close
    // to the preset bar once everything below it got tightened up) ---
    const float headerH = 36.0f;

    // --- preset bar: full width, right below the header and above the cat
    // mascot (rather than below the cat, per sideous-noise) - keeps it as
    // the very first thing under the title, uncluttered by the mascot/
    // panels below it ---
    const float presetBarY = margin + headerH + gap;
    const float presetBarH = 34.0f;
    {
        L.presetBar.accent = kPrideRed;
        const float y = presetBarY, h = presetBarH;
        const float barLeftX = margin;
        const float barRightX = width - margin;

        L.presetBar.prev = { barLeftX, y, 36.0f, h, "<", kPrideRed };
        L.presetBar.next = { barLeftX + 36.0f + 8.0f, y, 36.0f, h, ">", kPrideRed };

        const float delX = barRightX - 90.0f;
        const float saveX = delX - 8.0f - 80.0f;
        L.presetBar.save = { saveX, y, 80.0f, h, "SAVE", kPrideRed };
        L.presetBar.del  = { delX, y, 90.0f, h, "DELETE", kAccentCoral };

        L.presetBar.nameX = L.presetBar.next.x + 36.0f + 14.0f;
        L.presetBar.nameY = y;
        L.presetBar.nameW = saveX - 14.0f - L.presetBar.nameX;
        L.presetBar.nameH = h;
    }

    // --- cat mascot, centered, top of panel (the visual centerpiece) ---
    const float catW = 130.0f;
    const float catH = catW * (float)kUmbraCatHeight / (float)kUmbraCatWidth;
    L.cat = { (width - catW) * 0.5f, presetBarY + presetBarH + gap, catW, catH };

    const float colW = (width - margin * 2.0f - gap) / 2.0f;
    const float colBx = margin + colW + gap;

    // --- row 1: Voice | Formants ---
    const float row1Y = L.cat.y + L.cat.h + gap;
    const float row1RadiusVoice = 22.0f, row1RadiusFormants = 24.0f;
    const float row1H = std::max(knobPanelHeight(row1RadiusVoice), knobPanelHeight(row1RadiusFormants));
    L.panels.push_back({ margin, row1Y, colW, row1H, "VOICE",    kPrideRed });
    L.panels.push_back({ colBx,  row1Y, colW, row1H, "FORMANTS", kPrideOrange });

    addKnobRow(L.knobs, margin, colW, knobRowCenterY(row1Y, row1RadiusVoice), row1RadiusVoice, kPrideRed,
               { { kParamVowel, "VOWEL" }, { kParamVowelDrift, "DRIFT" }, { kParamGrowl, "GROWL" } });
    addKnobRow(L.knobs, colBx, colW, knobRowCenterY(row1Y, row1RadiusFormants), row1RadiusFormants, kPrideOrange,
               { { kParamFormantResonance, "RESO" }, { kParamFormantDrive, "DRIVE" } });

    // --- row 2: Swoop & Vibrato | Master (paired side by side - each only
    // has a couple of knobs, no reason for either to claim a full row) ---
    const float row2Y = row1Y + row1H + gap;
    const float row2Radius = 20.0f;
    const float row2H = knobPanelHeight(row2Radius);
    L.panels.push_back({ margin, row2Y, colW, row2H, "SWOOP & VIBRATO", kPrideYellow });
    L.panels.push_back({ colBx,  row2Y, colW, row2H, "MASTER",          kPrideBlue });
    addKnobRow(L.knobs, margin, colW, knobRowCenterY(row2Y, row2Radius), row2Radius, kPrideYellow,
               {
                   { kParamSwoopAmount,  "SWOOP AMT" },
                   { kParamSwoopTime,    "SWOOP TIME" },
                   { kParamVibratoRate,  "VIB RATE" },
                   { kParamVibratoDepth, "VIB DEPTH" },
               });
    addKnobRow(L.knobs, colBx, colW, knobRowCenterY(row2Y, row2Radius), row2Radius, kPrideBlue,
               { { kParamMasterVolume, "VOLUME" }, { kParamMasterDrive, "DRIVE" } });

    // --- row 3: Envelope (full width, 6 knobs + graph) ---
    const float row3Y = row2Y + row2H + gap;
    const float row3Radius = 20.0f;
    const float graphGap = 22.0f, graphH = 38.0f;
    const float row3H = 34.0f + row3Radius * 2.0f + graphGap + graphH + 10.0f;
    L.panels.push_back({ margin, row3Y, width - margin * 2.0f, row3H, "ENVELOPE", kPrideGreen });
    const float row3KnobY = knobRowCenterY(row3Y, row3Radius);
    addKnobRow(L.knobs, margin, width - margin * 2.0f, row3KnobY, row3Radius, kPrideGreen,
               {
                   { kParamAmpAttack,          "ATTACK" },
                   { kParamAmpDecay,            "DECAY" },
                   { kParamAmpSustain,          "SUSTAIN" },
                   { kParamAmpRelease,          "RELEASE" },
                   { kParamAmpCurve,            "CURVE" },
                   { kParamVelocitySensitivity, "VELOCITY" },
               });
    L.envelopeGraphs.push_back({
        kParamAmpAttack, kParamAmpDecay, kParamAmpSustain, kParamAmpRelease, kParamAmpCurve,
        margin + 12.0f, row3KnobY + row3Radius + graphGap, width - margin * 2.0f - 24.0f, graphH, kPrideGreen });

    L.height = row3Y + row3H + margin;
    return L;
}

// -----------------------------------------------------------------------------------------------------------
// value <-> knob-rotation mapping (shared by drawing and mouse-drag hit logic)

inline float paramToNormalized(uint32_t param, float value) noexcept
{
    const ParamInfo& info = getParamInfo(param);
    float t;
    if (info.shape == ParamShape::Logarithmic)
    {
        const float lo = info.min > 0.0f ? info.min : 1e-6f;
        const float hi = info.max > lo ? info.max : lo * 1.0001f;
        const float v = value < lo ? lo : (value > hi ? hi : value);
        t = (float)(std::log(v / lo) / std::log(hi / lo));
    }
    else
    {
        t = (value - info.min) / (info.max - info.min);
    }
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

inline float normalizedToParam(uint32_t param, float t) noexcept
{
    const ParamInfo& info = getParamInfo(param);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    if (info.shape == ParamShape::Logarithmic)
    {
        const float lo = info.min > 0.0f ? info.min : 1e-6f;
        const float hi = info.max > lo ? info.max : lo * 1.0001f;
        return lo * std::pow(hi / lo, t);
    }
    return info.min + (info.max - info.min) * t;
}

// -----------------------------------------------------------------------------------------------------------
// value formatting

inline void formatParamValue(uint32_t index, float value, char* out, size_t outSize) noexcept
{
    switch (index)
    {
    case kParamVowel:
    {
        static const char* const names[5] = { "AH", "EH", "EE", "OH", "OO" };
        float v = value < 0.0f ? 0.0f : (value > 4.0f ? 4.0f : value);
        const int idx0 = std::min((int)v, 4);
        const int idx1 = std::min(idx0 + 1, 4);
        const float frac = v - (float)idx0;
        if (idx0 == idx1 || frac < 0.12f)
            std::snprintf(out, outSize, "%s", names[idx0]);
        else if (frac > 0.88f)
            std::snprintf(out, outSize, "%s", names[idx1]);
        else
            std::snprintf(out, outSize, "%s-%s", names[idx0], names[idx1]);
        return;
    }
    case kParamSwoopAmount:
        std::snprintf(out, outSize, "%+.1fst", value);
        return;
    case kParamVibratoDepth:
        std::snprintf(out, outSize, "%.2fst", value);
        return;
    default:
        break;
    }

    const ParamInfo& info = getParamInfo(index);

    if (std::strcmp(info.unit, "Hz") == 0)
    {
        if (value >= 1000.0f)
            std::snprintf(out, outSize, "%.2fk", value / 1000.0);
        else
            std::snprintf(out, outSize, "%.2f", value);
    }
    else if (std::strcmp(info.unit, "s") == 0)
    {
        if (value < 1.0f)
            std::snprintf(out, outSize, "%.0fms", value * 1000.0);
        else
            std::snprintf(out, outSize, "%.2fs", value);
    }
    else
    {
        std::snprintf(out, outSize, "%.2f", value);
    }
}

// -----------------------------------------------------------------------------------------------------------
// drawing helpers

inline void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI_2, 0.0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0.0,      M_PI_2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI_2,   M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI,     3.0 * M_PI_2);
    cairo_close_path(cr);
}

// cairo_show_text() leaves a dangling current point; if the next drawing call
// is cairo_arc()/cairo_line_to() without an explicit new subpath, cairo will
// implicitly connect from that stray point. Always reset the path after text.
inline void drawText(cairo_t* cr, const char* text, double x, double y)
{
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
    cairo_new_path(cr);
}

inline void centeredText(cairo_t* cr, const char* text, double cx, double cy)
{
    cairo_text_extents_t ext;
    cairo_text_extents(cr, text, &ext);
    drawText(cr, text, cx - ext.width / 2.0 - ext.x_bearing, cy - ext.height / 2.0 - ext.y_bearing);
}

inline void setFont(cairo_t* cr, double size, bool bold = true)
{
    cairo_select_font_face(cr, "monospace",
                            CAIRO_FONT_SLANT_NORMAL,
                            bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
}

// -----------------------------------------------------------------------------------------------------------
// Umbra the cat - loaded once from the embedded PNG bytes (no file I/O, see
// UmbraCatImage.hpp) and painted with its own original umbra.svg colors
// (black body, gold ear/eye patches, teal outline) rather than a flat tint -
// a first version painted this in a 6-stripe Pride gradient via
// cairo_mask_surface(), but that made the eye-brightening overlay below
// (paintUmbraEyes()) nearly invisible since the gold eyes sat right on top of
// an already-bright yellow gradient stripe. Original colors give the eye
// glow somewhere dark to stand out against instead.

struct PngReadState { const unsigned char* data; std::size_t size; std::size_t pos; };

inline cairo_status_t catPngReadFunc(void* closure, unsigned char* data, unsigned int length) noexcept
{
    PngReadState* state = (PngReadState*)closure;
    if (state->pos + length > state->size)
        return CAIRO_STATUS_READ_ERROR;
    std::memcpy(data, state->data + state->pos, length);
    state->pos += length;
    return CAIRO_STATUS_SUCCESS;
}

inline cairo_surface_t* getCatSurface() noexcept
{
    static cairo_surface_t* surface = nullptr;
    if (surface == nullptr)
    {
        PngReadState state{ kUmbraCatPng, kUmbraCatPngSize, 0 };
        surface = cairo_image_surface_create_from_png_stream(catPngReadFunc, &state);
    }
    return surface;
}

inline void paintUmbra(cairo_t* cr, const CatBox& box)
{
    cairo_surface_t* cat = getCatSurface();
    if (cat == nullptr || cairo_surface_status(cat) != CAIRO_STATUS_SUCCESS)
        return;

    // scale from the actually-decoded PNG width/height rather than the
    // hand-written kUmbraCatWidth/kUmbraCatHeight constants - the eyes PNG
    // (UmbraEyesImage.hpp) was rasterized identically but its header
    // constants read 1px different from the cat's, so trusting the decoded
    // surface here is what keeps the two images pixel-aligned when composited.
    const double catW = (double)cairo_image_surface_get_width(cat);
    const double scale = (double)box.w / catW;

    cairo_save(cr);
    cairo_translate(cr, box.x, box.y);
    cairo_scale(cr, scale, scale);

    cairo_set_source_surface(cr, cat, 0.0, 0.0);
    cairo_paint(cr);

    cairo_restore(cr);
}

// -----------------------------------------------------------------------------------------------------------
// Umbra's eyes - a second PNG (path2/path3 from the source SVG, rasterized
// at the identical canvas size/transform as the cat body, see
// UmbraEyesImage.hpp) composited on top of paintUmbra() at the same box, so
// it lines up with the body automatically. Tinted solid gold->yellow by the
// live output level instead of the cat body's rainbow gradient.

inline cairo_surface_t* getEyesSurface() noexcept
{
    static cairo_surface_t* surface = nullptr;
    if (surface == nullptr)
    {
        PngReadState state{ kUmbraEyesPng, kUmbraEyesPngSize, 0 };
        surface = cairo_image_surface_create_from_png_stream(catPngReadFunc, &state);
    }
    return surface;
}

inline void paintUmbraEyes(cairo_t* cr, const CatBox& box, float level)
{
    cairo_surface_t* eyes = getEyesSurface();
    if (eyes == nullptr || cairo_surface_status(eyes) != CAIRO_STATUS_SUCCESS)
        return;

    level = level < 0.0f ? 0.0f : (level > 1.0f ? 1.0f : level);

    // dim base gold (the eye shape's original SVG fill, #8f6d00) interpolated
    // toward a hot, vivid yellow glow (#FFF033) as the live output level rises
    static constexpr Color kEyeDim    { 0.561, 0.427, 0.000 }; // #8f6d00
    static constexpr Color kEyeBright { 1.000, 0.941, 0.200 }; // #FFF033
    const Color eyeColor {
        kEyeDim.r + (kEyeBright.r - kEyeDim.r) * (double)level,
        kEyeDim.g + (kEyeBright.g - kEyeDim.g) * (double)level,
        kEyeDim.b + (kEyeBright.b - kEyeDim.b) * (double)level,
    };

    // scale from the decoded surface, same reasoning as paintUmbra() above -
    // both PNGs were rasterized at the same size, so this and paintUmbra()'s
    // scale end up identical, keeping the two images pixel-aligned
    const double eyesW = (double)cairo_image_surface_get_width(eyes);
    const double scale = (double)box.w / eyesW;

    cairo_save(cr);
    cairo_translate(cr, box.x, box.y);
    cairo_scale(cr, scale, scale);

    // subtle glow at high level: a low-alpha, slightly larger copy painted
    // first so bright eyes read as glowing rather than just flat-colored
    if (level > 0.35f)
    {
        const double glowAlpha = (double)((level - 0.35f) / 0.65f) * 0.35;
        const double glowScale = 1.12;
        const double eyesH = (double)cairo_image_surface_get_height(eyes);
        cairo_save(cr);
        cairo_translate(cr, eyesW * 0.5, eyesH * 0.5);
        cairo_scale(cr, glowScale, glowScale);
        cairo_translate(cr, -eyesW * 0.5, -eyesH * 0.5);
        cairo_set_source_rgba(cr, kEyeBright.r, kEyeBright.g, kEyeBright.b, glowAlpha);
        cairo_mask_surface(cr, eyes, 0.0, 0.0);
        cairo_restore(cr);
    }

    cairo_set_source_rgb(cr, eyeColor.r, eyeColor.g, eyeColor.b);
    cairo_mask_surface(cr, eyes, 0.0, 0.0);

    cairo_restore(cr);
}

// -----------------------------------------------------------------------------------------------------------
// paint

struct PaintState
{
    const float* values = nullptr;
    int hoverKnob = -1;
    int dragKnob = -1;
    const bool* autoShowValue = nullptr;

    // preset bar - pure UI-side state, not backed by any DPF parameter (see
    // ui/PresetStore.hpp)
    const char* presetName = "INIT";
    bool presetIsUnsaved = false; // true when a param has changed since the
                                   // displayed preset was last loaded/saved
    bool presetEditingName = false;
    const char* presetEditBuffer = "";
    int hoverPresetButton = -1;   // 0=prev,1=next,2=save,3=delete, or -1
    bool presetDeleteEnabled = false; // false while on INIT - nothing to delete
};

inline void paintKnob(cairo_t* cr, const Knob& k, float value, bool hovered, bool dragging, bool autoShow)
{
    const float t = paramToNormalized(k.param, value);

    const double startAngle = M_PI * 0.75;
    const double sweep = M_PI * 1.5;
    const double valueAngle = startAngle + sweep * t;

    cairo_set_line_width(cr, 5.0);
    setColor(cr, kKnobRing);
    cairo_arc(cr, k.cx, k.cy, k.radius, startAngle, startAngle + sweep);
    cairo_stroke(cr);

    setColor(cr, k.accent, dragging ? 1.0 : (hovered ? 0.9 : 0.85));
    cairo_arc(cr, k.cx, k.cy, k.radius, startAngle, valueAngle);
    cairo_stroke(cr);

    setColor(cr, kKnobBody);
    cairo_arc(cr, k.cx, k.cy, k.radius - 6.0, 0.0, 2.0 * M_PI);
    cairo_fill(cr);

    if (hovered || dragging)
    {
        setColor(cr, k.accent, 0.22);
        cairo_arc(cr, k.cx, k.cy, k.radius - 6.0, 0.0, 2.0 * M_PI);
        cairo_fill(cr);
    }

    setColor(cr, kTextMain);
    cairo_set_line_width(cr, 3.0);
    const double px = k.cx + std::cos(valueAngle) * (k.radius - 10.0);
    const double py = k.cy + std::sin(valueAngle) * (k.radius - 10.0);
    cairo_move_to(cr, k.cx, k.cy);
    cairo_line_to(cr, px, py);
    cairo_stroke(cr);

    setFont(cr, 10.5);
    setColor(cr, kTextDim);
    centeredText(cr, k.label, k.cx, k.cy + k.radius + 14.0);

    if (dragging || autoShow)
    {
        char buf[32];
        formatParamValue(k.param, value, buf, sizeof(buf));
        setFont(cr, 11.5);
        setColor(cr, kTextMain);
        centeredText(cr, buf, k.cx, k.cy - k.radius - 12.0);
    }
}

inline float envelopeCurveShape(float t, float curveAmount) noexcept
{
    const float exponent = std::pow(4.0f, -curveAmount);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return std::pow(t, exponent);
}

inline void paintEnvelopeGraph(cairo_t* cr, const EnvelopeGraph& eg, const float* values)
{
    const float sustain = values[eg.sustainParam];
    const float curve = values[eg.curveParam];

    const float attackFrac = 0.25f, decayFrac = 0.25f, holdFrac = 0.20f, releaseFrac = 0.30f;

    roundedRect(cr, eg.x, eg.y, eg.w, eg.h, 4.0);
    setColor(cr, kBg, 0.7);
    cairo_fill(cr);

    const int steps = 14;
    const double x0 = eg.x, y0 = eg.y, w = eg.w, h = eg.h;
    auto plotX = [&](float frac) { return x0 + w * (double)frac; };
    auto plotY = [&](float level) { return y0 + h * (1.0 - (double)level); };

    cairo_new_path(cr);
    cairo_move_to(cr, plotX(0.0f), plotY(0.0f));

    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float)i / (float)steps;
        const float level = envelopeCurveShape(t, curve);
        cairo_line_to(cr, plotX(attackFrac * t), plotY(level));
    }
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float)i / (float)steps;
        const float shaped = envelopeCurveShape(t, curve);
        const float level = 1.0f + (sustain - 1.0f) * shaped;
        cairo_line_to(cr, plotX(attackFrac + decayFrac * t), plotY(level));
    }
    cairo_line_to(cr, plotX(attackFrac + decayFrac + holdFrac), plotY(sustain));
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float)i / (float)steps;
        const float shaped = envelopeCurveShape(t, curve);
        const float level = sustain * (1.0f - shaped);
        cairo_line_to(cr, plotX(attackFrac + decayFrac + holdFrac + releaseFrac * t), plotY(level));
    }

    setColor(cr, eg.accent, 1.0);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke_preserve(cr);

    cairo_line_to(cr, plotX(1.0f), plotY(0.0f));
    cairo_line_to(cr, plotX(0.0f), plotY(0.0f));
    cairo_close_path(cr);
    setColor(cr, eg.accent, 0.22);
    cairo_fill(cr);
}

inline void paintButton(cairo_t* cr, const Button& b, bool hovered, bool enabled)
{
    roundedRect(cr, b.x, b.y, b.w, b.h, 4.0);
    setColor(cr, kBtnBg, enabled ? (hovered ? 1.0 : 0.9) : 0.5);
    cairo_fill_preserve(cr);
    setColor(cr, b.accent, enabled ? 1.0 : 0.35);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    setFont(cr, 11.5);
    setColor(cr, enabled ? kTextMain : kTextDim);
    centeredText(cr, b.label, b.x + b.w / 2.0, b.y + b.h / 2.0);
}

inline void paintPresetBar(cairo_t* cr, const PresetBarLayout& pb, const PaintState& state)
{
    paintButton(cr, pb.prev, state.hoverPresetButton == 0, true);
    paintButton(cr, pb.next, state.hoverPresetButton == 1, true);
    paintButton(cr, pb.save, state.hoverPresetButton == 2, true);
    paintButton(cr, pb.del,  state.hoverPresetButton == 3, state.presetDeleteEnabled);

    roundedRect(cr, pb.nameX, pb.nameY, pb.nameW, pb.nameH, 4.0);
    setColor(cr, state.presetEditingName ? kKnobBody : kBtnBg, 0.95);
    cairo_fill_preserve(cr);
    setColor(cr, state.presetEditingName ? pb.accent : kPanelEdge, state.presetEditingName ? 1.0 : 0.6);
    cairo_set_line_width(cr, state.presetEditingName ? 2.0 : 1.5);
    cairo_stroke(cr);

    setFont(cr, 13.0);
    if (state.presetEditingName)
    {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "%s_", state.presetEditBuffer);
        setColor(cr, kTextMain);
        drawText(cr, buf, pb.nameX + 12.0, pb.nameY + pb.nameH / 2.0 + 4.5);
    }
    else
    {
        setColor(cr, state.presetIsUnsaved ? kAccentCoral : kTextMain);
        char buf[40];
        std::snprintf(buf, sizeof(buf), "%s%s", state.presetName, state.presetIsUnsaved ? " *" : "");
        drawText(cr, buf, pb.nameX + 12.0, pb.nameY + pb.nameH / 2.0 + 4.5);
    }
}

inline void paint(cairo_t* cr, const Layout& L, const PaintState& state)
{
    setColor(cr, kBg);
    cairo_paint(cr);

    setFont(cr, 24.0);
    setColor(cr, kPrideRed);
    drawText(cr, "SIDEOUS UMBRA", 20.0, 32.0);

    setFont(cr, 9.5, false);
    setColor(cr, kTextDim);
    {
        const char* subtitle = "VOWEL-MORPHING CAT SYNTH";
        cairo_text_extents_t ext;
        cairo_text_extents(cr, subtitle, &ext);
        drawText(cr, subtitle, L.width - 20.0 - ext.width, 20.0);
    }

    paintPresetBar(cr, L.presetBar, state);

    paintUmbra(cr, L.cat);
    paintUmbraEyes(cr, L.cat, state.values != nullptr ? state.values[kParamOutLevel] : 0.0f);

    for (const PanelBox& p : L.panels)
    {
        roundedRect(cr, p.x, p.y, p.w, p.h, 8.0);
        setColor(cr, kPanelBg);
        cairo_fill_preserve(cr);
        setColor(cr, kPanelEdge, 0.8);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);

        if (p.title != nullptr)
        {
            setFont(cr, 12.0);
            setColor(cr, titleTextColor(p.accent));
            drawText(cr, p.title, p.x + 14.0, p.y + 22.0);
        }
    }

    for (const EnvelopeGraph& eg : L.envelopeGraphs)
        paintEnvelopeGraph(cr, eg, state.values);

    for (size_t i = 0; i < L.knobs.size(); ++i)
    {
        const Knob& k = L.knobs[i];
        const bool autoShow = state.autoShowValue != nullptr && state.autoShowValue[k.param];
        paintKnob(cr, k, state.values[k.param], (int)i == state.hoverKnob, (int)i == state.dragKnob, autoShow);
    }
}

} // namespace ui
} // namespace sideous
