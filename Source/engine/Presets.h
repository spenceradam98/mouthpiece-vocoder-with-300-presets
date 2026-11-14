// Factory presets: 13 categories, 300+ programs. Each preset is a short "key=value" recipe applied over
// its category's base recipe, which is applied over the parameter defaults. Keys are parameter ids from
// Params.h; choice values may be given by label (spaces dropped, case ignored), e.g. carrier=AutoPitch.
#pragma once

#include "Params.h"
#include <string>
#include <vector>

namespace mp
{
struct PresetCategory
{
    const char* name;
    const char* base;
};

struct PresetDef
{
    int category;
    const char* name;
    const char* recipe;
};

const PresetCategory* presetCategories (int& count);
const PresetDef* presetDefs (int& count);

// Applies a recipe on top of p. Returns false and fills `error` for unknown keys, bad values or out-of-range numbers.
bool applyRecipe (Params& p, const char* recipe, std::string* error = nullptr);

// Full parameter set for preset index (defaults + category base + preset recipe).
Params presetParams (int index);
int numPresets();
} // namespace mp
