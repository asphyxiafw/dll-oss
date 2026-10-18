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
    bool forceReload, std::optional<uint32_t> optLoadOrder,
    cg::TextureSetLayerType textureType) {
    // The hidden return-storage pointer is required by this MSVC x64 ABI.
    if (result == nullptr || funcOriginal == nullptr) return nullptr;

    // getTexture may recursively request other textures. Only the outer call
    // emits GetTextureEvent, so listeners cannot recursively trigger themselves.
    static thread_local bool inCallback = false;
    if (inCallback) {
        return funcOriginal(_this, result, location, forceReload, optLoadOrder, textureType);
    }

    struct RecursionGuard {
        bool& flag;
        explicit RecursionGuard(bool& value) : flag(value) { flag = true; }
        ~RecursionGuard() { flag = false; }
    };

    RecursionGuard guard(inCallback);
    TexturePtr* retTexture = funcOriginal(_this, result, location, forceReload, optLoadOrder, textureType);

    // Some callers may request a texture without a resource key. Preserve the
    // game's result but don't hand invalid data to mod listeners.
    if (retTexture == nullptr || location == nullptr) return retTexture;

    auto* textureData = retTexture->clientTexture.get();
    auto event = nes::make_holder<GetTextureEvent>(location, textureData);
    // Keep the recursion guard active while listeners run as well: a listener
    // may request another texture, which should call the original without
    // recursively broadcasting GetTextureEvent.
    eventMgr.trigger(event);

    return retTexture;
}
