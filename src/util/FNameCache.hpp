#pragma once
#include <string>
#include <unordered_map>

class FNameCache {
	std::unordered_map<std::wstring, FName *> m_namesToCache;

public:
	void registerName(const wchar_t *name, FName *memo) { m_namesToCache[name] = memo; }
	void registerNames(const std::unordered_map<std::wstring, FName *> &names);
	void cacheNames();
};

extern class FNameCache g_fnameCache;
