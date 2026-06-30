#include "OwnerTokenStorage.h"
#include <Preferences.h>

namespace hub::core::utils {

Result OwnerTokenStorage::save(const String& token) {
    Preferences prefs;
    if (!prefs.begin(m_namespaceName, false)) {
        return Result::Error;
    }
    bool ok = prefs.putString("token", token);
    prefs.end();
    return ok ? Result::Ok : Result::Error;
}

Result OwnerTokenStorage::load(String& outToken) {
    Preferences prefs;
    if (!prefs.begin(m_namespaceName, true)) {
        return Result::Error;
    }
    if (!prefs.isKey("token")) {
        prefs.end();
        return Result::NotInitialized;
    }
    outToken = prefs.getString("token");
    prefs.end();
    return Result::Ok;
}

bool OwnerTokenStorage::hasOwner() const {
    Preferences prefs;
    if (!prefs.begin(m_namespaceName, true)) {
        return false;
    }
    bool exists = prefs.isKey("token");
    prefs.end();
    return exists;
}

bool OwnerTokenStorage::verify(const char* token) const {
    Preferences prefs;
    if (!prefs.begin(m_namespaceName, true)) {
        return false;
    }
    if (!prefs.isKey("token")) {
        prefs.end();
        return false;
    }
    String stored = prefs.getString("token");
    prefs.end();
    return stored == String(token);
}

void OwnerTokenStorage::clear() {
    Preferences prefs;
    if (!prefs.begin(m_namespaceName, false)) {
        return;
    }
    prefs.clear();
    prefs.end();
}

} // namespace hub::core::utils