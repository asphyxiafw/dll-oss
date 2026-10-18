#pragma once

#include <array>
#include "../Hook.hpp"
#include "../../../../SDK/Client/Render/TextureGroup.hpp"
#include "../../../../SDK/Client/Render/ResourceLocation.hpp"
#include "../../../../SDK/Client/Render/TexturePtr.hpp"

class TextureGroup_getTextureHook : public Hook {
private:
    static TexturePtr *TextureGroup_getTextureCallback(
        TextureGroup* _this,
        TexturePtr* result,
        ResourceLocation* location,
        bool forceReload,
        std::optional<uint32_t> optLoadOrder,
        cg::TextureSetLayerType textureType
    );

public:
    // ABI mirrors TextureGroupBase::getTexture: the hidden return-storage
    // pointer is followed by location, forceReload, optional load order, and layer type.
    using original = TexturePtr *(__thiscall *)(
        TextureGroup* _this,
        TexturePtr* result,
        ResourceLocation* location,
        bool forceReload,
        std::optional<uint32_t> optLoadOrder,
        cg::TextureSetLayerType textureType
    );

    static inline original funcOriginal = nullptr;

    TextureGroup_getTextureHook();

    void enableHook() override;
};
