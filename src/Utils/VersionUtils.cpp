#include "VersionUtils.hpp"

#include <Client/Client.hpp>
#include <Utils/Memory/Game/Offset/OffsetInit.hpp>
#include <Utils/Memory/Game/Sig/SigInit.hpp>
#include <Utils/Memory/Game/SignatureAndOffsetManager.hpp>
#include <Utils/WinrtUtils.hpp>

std::vector<std::pair<std::string, std::pair<std::function<void()>, std::function<void()>>>> VersionUtils::versions;

std::string VersionUtils::getFormattedVersion() {
    Version currentVersion = WinrtUtils::impl::getGameVersion();
    std::string version = WinrtUtils::impl::toRawString(currentVersion);

    Logger::debug(version);

    if (version == "1.21.131") { // some sigs changed
        return "1.21.131";
    }

    if (currentVersion.build == 130) { // dude
        return "1.21.13";
    }

    if (version == "1.21.202") { // i cba for now
        // 1.21.202.0 - 1.21.2
        // 1.21.2003.0 - 1.21.20
        return "1.21.02";
    }

    if (version == "1.21.3") { // i cba for now #2
        // 1.21.3.0 - 1.21.0 ".3"
        // 1.21.3003.0 - 1.21.30
        return "1.21.03";
    }

    std::vector<std::string> parts;
    std::istringstream iss(version);
    std::string part;

    while (std::getline(iss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.size() < 2) {
        return version;
    }

    std::string formattedVersion = parts[0] + "." + parts[1];
    if (parts.size() > 2) {
        formattedVersion += "." + parts[2].substr(0, parts[2].size() - 1);
    }

    return formattedVersion;
}

void VersionUtils::initialize() {
    versions = {
        {"1.26.3", {SigInit::init260, OffsetInit::init260}},
        {"1.26.2", {SigInit::init260, OffsetInit::init260}},
        {"1.26.1", {SigInit::init260, OffsetInit::init260}},
        {"1.26.0", {SigInit::init260, OffsetInit::init260}},
        {"1.21.132", {SigInit::init21131, OffsetInit::init21130}},
        {"1.21.131", {SigInit::init21131, OffsetInit::init21130}},
        {"1.21.130", {SigInit::init21130, OffsetInit::init21130}},
        {"1.21.13", {SigInit::init21130, OffsetInit::init21130}},
        {"1.21.12", {SigInit::init21120, OffsetInit::init21120}},
        {"1.21.11", {SigInit::init21110, OffsetInit::init21110}},
        {"1.21.10", {SigInit::init21100, OffsetInit::init21100}},
        {"1.21.9", {SigInit::init2190, OffsetInit::init2190}},
        {"1.21.8", {SigInit::init2180, OffsetInit::init2180}},
        {"1.21.7", {SigInit::init2170, OffsetInit::init2170}},
        {"1.21.6", {SigInit::init2160, OffsetInit::init2160}},
        {"1.21.5", {SigInit::init2150, OffsetInit::init2150}},
        {"1.21.4", {SigInit::init2140, OffsetInit::init2140}},
        {"1.21.3", {SigInit::init2130, OffsetInit::init2130}},
        {"1.21.2", {SigInit::init2120, OffsetInit::init2120}},
        {"1.21.02", {SigInit::init2102, OffsetInit::init2100}},
        {"1.21.1", {SigInit::init2100, OffsetInit::init2100}},
        {"1.21.03", {SigInit::init2100, OffsetInit::init2100}},
        {"1.20.8", {SigInit::init2080, OffsetInit::init2080}},
        {"1.20.7", {SigInit::init2070, OffsetInit::init2070}},
        {"1.20.6", {SigInit::init2060, OffsetInit::init2060}},
        {"1.20.5", {SigInit::init2050, OffsetInit::init2050}},
        {"1.20.4", {SigInit::init2040, OffsetInit::init2040}},
        {"1.20.3", {SigInit::init2030, OffsetInit::init2030}}
    };

    std::ranges::reverse(versions);
}

bool VersionUtils::isSupported(const std::string& version) {
    if (std::ranges::any_of(versions, [&version](const auto& p) {
        return p.first == version;
    })) {
        return true;
    }

    std::string supported;
    for (const auto& [v, _] : versions) {
        if (!supported.empty()) supported += ", ";
        supported += v;
    }
    Logger::debug("unsupported {} vs supported {}", version, supported);
    return false;
}

void VersionUtils::addData() {
    std::thread t1([&]() {
        for (const auto& [version, loaders] : versions) {
            loaders.first();
            if (version == Client::version) break;
        }
    });

    std::thread t2([&]() {
        for (const auto& [version, loaders] : versions) {
            loaders.second();
            if (version == Client::version) break;
        }
    });

    t1.join();
    t2.join();

    Mgr.scanAllSignatures();

    const auto total = Mgr.getSignatureCount();
    const auto resolved = Mgr.getResolvedSignatureCount();
    const auto missing = Mgr.getUnresolvedSignatureNames();

    Logger::success("Signature scan for Minecraft {}: {}/{} resolved",
                    Client::version, resolved, total);

    if (!missing.empty()) {
        Logger::warn("{} signatures were not found; some features may be unavailable",
                     missing.size());

        // Keep startup logs useful instead of dumping hundreds of entries.
        constexpr std::size_t maxNamesToLog = 12;
        const std::size_t countToLog = std::min(maxNamesToLog, missing.size());
        for (std::size_t i = 0; i < countToLog; ++i) {
            Logger::debug("Unresolved signature: {}", missing[i]);
        }
        if (missing.size() > countToLog) {
            Logger::debug("... and {} more unresolved signatures",
                          missing.size() - countToLog);
        }
    }
}

bool VersionUtils::checkAboveOrEqual(const int m, const int b) {
    static auto [major, minor, build, error] = WinrtUtils::impl::getGameVersion();
    if (minor > m)
        return true;
    if (minor == m)
        return build >= b;
    return false;
}

bool VersionUtils::checkAbove(const int m, const int b) {
    static auto [major, minor, build, error] = WinrtUtils::impl::getGameVersion();
    if (minor > m)
        return true;
    if (minor == m)
        return build > b;
    return false;
}

bool VersionUtils::checkBelowOrEqual(const int m, const int b) {
    static auto [major, minor, build, error] = WinrtUtils::impl::getGameVersion();
    if (minor < m)
        return true;
    if (minor == m)
        return build <= b;
    return false;
}

bool VersionUtils::checkBelow(const int m, const int b) {
    static auto [major, minor, build, error] = WinrtUtils::impl::getGameVersion();
    if (minor < m)
        return true;
    if (minor == m)
        return build < b;
    return false;
}

bool VersionUtils::checkBetweenOrEqual(const int m1, const int b1, const int m2, const int b2) {
    return checkAboveOrEqual(m1, b1) && checkBelowOrEqual(m2, b2);
}

bool VersionUtils::checkBetween(const int m1, const int b1, const int m2, const int b2) {
    return checkAbove(m1, b1) && checkBelow(m2, b2);
}

bool VersionUtils::checkEqual(const int m, const int b) {
    static auto [major, minor, build, error] = WinrtUtils::impl::getGameVersion();
    return minor == m && build == b;
}
