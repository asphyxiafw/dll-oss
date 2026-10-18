#include "Hook.hpp"
#include <libhat/Scanner.hpp>
#include <utility>

Hook::Hook(std::string name, uintptr_t address) {
    this->name = std::move(name);
    this->address = address;
}

Hook::~Hook() = default;

void Hook::enableHook() {
}

bool Hook::autoHook(void *callbackPtr, void **funcOriginal) {
    if (address == 0) {
        Logger::warn("Skipped hook {} because its target signature did not resolve.", name);
        return false;
    }
    return this->manualHook(reinterpret_cast<void*>(address), callbackPtr, funcOriginal);
}

bool Hook::manualHook(void *hookAddr, void *callbackPtr, void **funcOriginal) {
    if (hookAddr == nullptr) {
        Logger::custom(fg(fmt::color::crimson), "Hook", "Failed to find address of {}", this->name);
        return false;
    }

    // Do not queue an address for enabling unless MinHook successfully created
    // the trampoline. Previously the create result was ignored, which could
    // make the logs claim a hook was installed when it was not.
    const MH_STATUS createStatus = MH_CreateHook(hookAddr, callbackPtr, funcOriginal);
    if (createStatus != MH_OK) {
        Logger::custom(fg(fmt::color::crimson), "Hook",
                       "MH_CreateHook failed for {} (status {})",
                       this->name, static_cast<int>(createStatus));
        return false;
    }

    const MH_STATUS queueStatus = MH_QueueEnableHook(hookAddr);
    if (queueStatus != MH_OK) {
        // The hook exists but is not queued for activation. Remove it so a
        // failed initialization does not leave a stale MinHook entry behind.
        MH_RemoveHook(hookAddr);
        Logger::custom(fg(fmt::color::crimson), "Hook",
                       "MH_QueueEnableHook failed for {} (status {})",
                       this->name, static_cast<int>(queueStatus));
        return false;
    }

    Logger::custom(fg(fmt::color::deep_sky_blue), "Hook", "Queued hook {}", this->name);
    return true;
}
