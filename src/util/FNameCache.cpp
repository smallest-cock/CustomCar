#include "pch.h"
#include "util/logging.hpp"
#include "FNameCache.hpp"

void FNameCache::registerNames(const std::unordered_map<std::wstring, FName *> &names) {
	m_namesToCache.insert(names.begin(), names.end());
}

void FNameCache::cacheNames() {
	if (m_namesToCache.empty())
		return;

	for (int32_t i = 0; i < GNames->size(); i++) {
		if (!GNames->at(i))
			continue;
		auto it = m_namesToCache.find(GNames->at(i)->Name);
		if (it != m_namesToCache.end() && it->second) {
			*(it->second) = i;
			m_namesToCache.erase(it);
		}
		if (m_namesToCache.empty()) {
			LOGWARNING("Found all registered FNames by index {} in GNames. Finna early exit...", i);
			return;
		}
	}

	if (!m_namesToCache.empty()) {
		LOGERROR("{} registered FNames weren't found in GNames:", m_namesToCache.size());
		for (const auto &[key, val] : m_namesToCache) {
			LOG(key);
		}
	}
}

FNameCache g_fnameCache{};
