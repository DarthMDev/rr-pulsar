#ifndef _PUL_RECENT_PLAYERS_STORE_
#define _PUL_RECENT_PLAYERS_STORE_

#include <kamek.hpp>
#include <core/rvl/RFL/RFLTypes.hpp>

namespace Pulsar {
namespace RecentPlayers {

static const u32 MAX_LICENSES = 4;
static const u32 MAX_ENTRIES_PER_LICENSE = 30;

struct RecentPlayerEntry {
    u64 friendCode;
    RFL::StoreData miiData;
    wchar_t name[12];
    float rating;
    bool isBattle;
    u64 timestampTicks;
    u32 encounterSeq;
    bool isValid;
};

struct LicenseHistory {
    u32 count;
    RecentPlayerEntry entries[MAX_ENTRIES_PER_LICENSE];
};

class RecentPlayersStore {
public:
    static RecentPlayersStore &Get();

    void Load();
    void Save();

    u32 GetCount(u32 licenseId) const;
    const RecentPlayerEntry *GetEntry(u32 licenseId, u32 index) const;

    void AddOrUpdate(u32 licenseId, const RecentPlayerEntry &entry);
    void ReconcileConfirmedFriends(u32 licenseId);

    static bool IsConfirmedFriend(u64 friendCode);

private:
    RecentPlayersStore();

    bool loaded;
    u32 globalEncounterSeq;
    LicenseHistory store[MAX_LICENSES];
};

void FormatRelativeTime(u64 timestampTicks, wchar_t *buffer, u32 bufferSize);

}  // namespace RecentPlayers
}  // namespace Pulsar

#endif
