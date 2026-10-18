#include "SignatureAndOffsetManager.hpp"

#include <Utils/Memory/Memory.hpp>
#include <algorithm>
#include <atomic>
#include <thread>

SignatureAndOffsetManager Mgr;

void SignatureAndOffsetManager::addSignature(unsigned int hash, const char* sig, const char* name) {
    sigs[hash] = { sig, name, 0 };
}

void SignatureAndOffsetManager::removeSignature(unsigned int hash) {
    sigs.erase(hash);
}

const char* SignatureAndOffsetManager::getSig(unsigned int hash) const {
    auto it = sigs.find(hash);
    return it != sigs.end() ? it->second.signature.c_str() : nullptr;
}

const char* SignatureAndOffsetManager::getSigName(unsigned int hash) const {
    auto it = sigs.find(hash);
    return it != sigs.end() ? it->second.name.c_str() : nullptr;
}

uintptr_t SignatureAndOffsetManager::getSigAddress(unsigned int hash) const {
    auto it = sigs.find(hash);
    return it != sigs.end() ? it->second.address : 0;
}

void SignatureAndOffsetManager::addOffset(unsigned int hash, int offset) {
    offsets[hash] = offset;
}

int SignatureAndOffsetManager::getOffset(unsigned int hash) const {
    auto it = offsets.find(hash);
    return it != offsets.end() ? it->second : 0; // Default to 0 if not found
}

std::size_t SignatureAndOffsetManager::getSignatureCount() const {
    return sigs.size();
}

std::size_t SignatureAndOffsetManager::getResolvedSignatureCount() const {
    return static_cast<std::size_t>(std::count_if(
        sigs.begin(), sigs.end(),
        [](const auto& entry) { return entry.second.address != 0; }));
}

std::vector<std::string> SignatureAndOffsetManager::getUnresolvedSignatureNames() const {
    std::vector<std::string> missing;
    for (const auto& [hash, signature] : sigs) {
        (void)hash;
        if (signature.address == 0) {
            missing.push_back(signature.name);
        }
    }
    std::ranges::sort(missing);
    return missing;
}

void SignatureAndOffsetManager::clear() {
    sigs.clear();
    offsets.clear();
}

void SignatureAndOffsetManager::scanAllSignatures() {
    // Take stable pointers before starting workers. The map is not mutated while
    // scanning, and each worker writes to a different SignatureData::address.
    std::vector<SignatureData*> entries;
    entries.reserve(sigs.size());
    for (auto& entry : sigs) {
        entries.push_back(&entry.second);
    }

    if (entries.empty()) {
        return;
    }

    // hardware_concurrency() is permitted to return 0. Clamp the worker count
    // to [1, number of signatures] so the scan always runs and avoids excess threads.
    const unsigned int availableThreads = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int numThreads = static_cast<unsigned int>(
        std::min<std::size_t>(availableThreads, entries.size()));

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    std::atomic<std::size_t> index{0};

    auto worker = [&entries, &index]() {
        while (true) {
            const std::size_t i = index.fetch_add(1, std::memory_order_relaxed);
            if (i >= entries.size()) {
                break;
            }

            auto* signature = entries[i];
            signature->address = Memory::findSig(signature->signature, signature->name);
        }
    };

    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
