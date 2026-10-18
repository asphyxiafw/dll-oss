#include "TextureGroup_getTextureHook.hpp"
#include "../../../../Utils/Memory/Game/SignatureAndOffsetManager.hpp"
#include "../../../Events/Render/GetTextureEvent.hpp"

TextureGroup_getTextureHook::TextureGroup_getTextureHook()
    : Hook("TextureGroup_getTexture", GET_SIG_ADDRESS("mce::TextureGroup::getTexture")) {}

void TextureGroup_getTextureHook::enableHook() {
    this->autoHook(reinterpret_cast<void*>(TextureGroup_getTextureCallback),
                   reinterpret_cast<void**>(&funcOriginal));
}

TexturePtr* TextureGroup_getTextureHook::TextureGroup_getTextureCallback(
    TextureGroup* _this, TexturePtr* result, ResourceLocation* location,
    bool forceReload, void* a5, int a6, void* a7) {
    // The hidden return-storage pointer is required by this MSVC x64 ABI.
    if (result == nullptr || funcOriginal == nullptr) return nullptr;

    // getTexture may recursively request other textures. Only the outer call
    // emits GetTextureEvent, so listeners cannot recursively trigger themselves.
    static thread_local bool inCallback = false;
    if (inCallback) {
        return funcOriginal(_this, result, location, forceReload, a5, a6, a7);
    }

    struct RecursionGuard {
        bool& flag;
        explicit RecursionGuard(bool& value) : flag(value) { flag = true; }
        ~RecursionGuard() { flag = false; }
    };

    TexturePtr* retTexture = nullptr;
    {
        RecursionGuard guard(inCallback);
        retTexture = funcOriginal(_this, result, location, forceReload, a5, a6, a7);
    }

    // Some callers may request a texture without a resource key. Preserve the
    // game's result but don't hand invalid data to mod listeners.
    if (retTexture == nullptr || location == nullptr) return retTexture;

    auto* textureData = reinterpret_cast<BedrockTextureData*>(&retTexture->clientTexture);
    auto event = nes::make_holder<GetTextureEvent>(location, textureData);
    eventMgr.trigger(event);

    return retTexture;
}
