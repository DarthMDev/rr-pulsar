#include <Network/RecentPlayers/RecentPlayersStore.hpp>
#include <IO/IO.hpp>
#include <PulsarSystem.hpp>
#include <MarioKartWii/RKNet/FriendMgr.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/RKSYS/RKSYSMgr.hpp>
#include <core/rvl/OS/OS.hpp>
#include <include/c_wchar.h>

namespace Pulsar {
namespace RecentPlayers {

static const u32 STORE_MAGIC = 'RPRP';
static const u16 STORE_VERSION = 1;

struct PackedHeader {
    u32 magic;
    u16 version;
    u16 licenseCount;
    u32 entryCountPerLicense;
    u32 globalEncounterSeq;
};

struct PackedEntry {
    u64 friendCode;
    RFL::StoreData miiData;
    wchar_t name[12];
    float rating;
    u8 isBattle;
    u8 isValid;
    u8 pad[2];
    u64 timestampTicks;
    u32 encounterSeq;
};

static char s_storePath[IOS::ipcMaxPath] __attribute__((aligned(32))) = {};

static const char *GetStorePath() {
    if (s_storePath[0] == '\0') {
        const System *sys = System::sInstance;
        if (!sys) return nullptr;
        snprintf(s_storePath, sizeof(s_storePath), "%s/RecentPlayers.pul", sys->GetModFolder());
    }
    return s_storePath;
}

RecentPlayersStore &RecentPlayersStore::Get() {
    static RecentPlayersStore s_instance;
    return s_instance;
}

RecentPlayersStore::RecentPlayersStore() : loaded(false), globalEncounterSeq(0) {
    memset(store, 0, sizeof(store));
}

void RecentPlayersStore::Load() {
    if (loaded) return;
    loaded = true;

    IO *io = IO::sInstance;
    const char *path = GetStorePath();
    if (!io || !path || !io->OpenFile(path, FILE_MODE_READ)) return;

    union {
        PackedHeader h;
        u8 pad[32];
    } hBuf __attribute__((aligned(32))) = {};

    if (io->Read(sizeof(PackedHeader), &hBuf.h) != sizeof(PackedHeader)) {
        io->Close();
        return;
    }

    if (hBuf.h.magic != STORE_MAGIC || hBuf.h.version != STORE_VERSION) {
        io->Close();
        return;
    }

    globalEncounterSeq = hBuf.h.globalEncounterSeq;
    u32 licCount = (hBuf.h.licenseCount < MAX_LICENSES) ? hBuf.h.licenseCount : MAX_LICENSES;
    u32 entriesPerLic = (hBuf.h.entryCountPerLicense < MAX_ENTRIES_PER_LICENSE) ? hBuf.h.entryCountPerLicense : MAX_ENTRIES_PER_LICENSE;

    for (u32 lic = 0; lic < licCount; ++lic) {
        store[lic].count = 0;
        for (u32 e = 0; e < entriesPerLic; ++e) {
            union {
                PackedEntry pe;
                u8 pad[32];
            } eBuf __attribute__((aligned(32))) = {};

            if (io->Read(sizeof(PackedEntry), &eBuf.pe) != static_cast<s32>(sizeof(PackedEntry))) {
                break;
            }

            if (eBuf.pe.isValid && eBuf.pe.friendCode != 0) {
                RecentPlayerEntry &entry = store[lic].entries[store[lic].count++];
                entry.friendCode = eBuf.pe.friendCode;
                entry.miiData = eBuf.pe.miiData;
                for (int k = 0; k < 12; ++k) entry.name[k] = eBuf.pe.name[k];
                entry.rating = eBuf.pe.rating;
                entry.isBattle = (eBuf.pe.isBattle != 0);
                entry.timestampTicks = eBuf.pe.timestampTicks;
                entry.encounterSeq = eBuf.pe.encounterSeq;
                entry.isValid = true;
            }
        }
    }

    io->Close();
}

void RecentPlayersStore::Save() {
    IO *io = IO::sInstance;
    const char *path = GetStorePath();
    if (!io || !path) return;

    PackedHeader header __attribute__((aligned(32))) = {};
    header.magic = STORE_MAGIC;
    header.version = STORE_VERSION;
    header.licenseCount = static_cast<u16>(MAX_LICENSES);
    header.entryCountPerLicense = MAX_ENTRIES_PER_LICENSE;
    header.globalEncounterSeq = globalEncounterSeq;

    if (!io->OpenFile(path, FILE_MODE_WRITE) && !io->CreateAndOpen(path, FILE_MODE_WRITE)) {
        return;
    }

    io->Overwrite(sizeof(PackedHeader), &header);

    for (u32 lic = 0; lic < MAX_LICENSES; ++lic) {
        for (u32 e = 0; e < MAX_ENTRIES_PER_LICENSE; ++e) {
            PackedEntry pe __attribute__((aligned(32))) = {};
            if (e < store[lic].count && store[lic].entries[e].isValid) {
                const RecentPlayerEntry &src = store[lic].entries[e];
                pe.friendCode = src.friendCode;
                pe.miiData = src.miiData;
                for (int k = 0; k < 12; ++k) pe.name[k] = src.name[k];
                pe.rating = src.rating;
                pe.isBattle = src.isBattle ? 1 : 0;
                pe.isValid = 1;
                pe.timestampTicks = src.timestampTicks;
                pe.encounterSeq = src.encounterSeq;
            }
            io->Overwrite(sizeof(PackedEntry), &pe);
        }
    }

    io->Close();
}

u32 RecentPlayersStore::GetCount(u32 licenseId) const {
    if (licenseId >= MAX_LICENSES) return 0;
    return store[licenseId].count;
}

const RecentPlayerEntry *RecentPlayersStore::GetEntry(u32 licenseId, u32 index) const {
    if (licenseId >= MAX_LICENSES || index >= store[licenseId].count) return nullptr;
    return &store[licenseId].entries[index];
}

bool RecentPlayersStore::IsConfirmedFriend(u64 friendCode) {
    if (friendCode == 0) return false;

    RKNet::FriendMgr *friendMgr = RKNet::FriendMgr::sInstance;
    if (friendMgr != nullptr && friendMgr->IsAvailable()) {
        s32 friendIdx = friendMgr->GetFriendIdx(friendCode);
        if (friendIdx >= 0) {
            typedef bool (*IsBuddyFriendFn)(const void *, u8);
            IsBuddyFriendFn isBuddy = reinterpret_cast<IsBuddyFriendFn>(0x8066375c);
            if (isBuddy != nullptr && isBuddy(friendMgr, static_cast<u8>(friendIdx))) {
                return true;
            }
            if (RKNet::Controller::sInstance != nullptr &&
                RKNet::Controller::sInstance->friends[friendIdx].hasAddedBack) {
                return true;
            }
        }
    }

    RKSYS::Mgr *rksys = RKSYS::Mgr::sInstance;
    if (rksys != nullptr && rksys->curLicenseId >= 0 && rksys->curLicenseId < static_cast<s32>(MAX_LICENSES)) {
        RKSYS::LicenseFriends &licFriends = rksys->licenses[rksys->curLicenseId].GetFriends();
        typedef BOOL (*IsBuddyFriendDataFn)(const DWC::AccFriendData *);
        IsBuddyFriendDataFn isBuddyData = reinterpret_cast<IsBuddyFriendDataFn>(0x800eb870);
        for (u32 i = 0; i < 30; ++i) {
            if (licFriends.friends[i].friendCode == friendCode) {
                if (isBuddyData != nullptr && isBuddyData(&licFriends.dwcFriends[i])) {
                    return true;
                }
            }
        }
    }

    return false;
}

void RecentPlayersStore::ReconcileConfirmedFriends(u32 licenseId) {
    if (licenseId >= MAX_LICENSES) return;
    Load();

    bool changed = false;
    LicenseHistory &lic = store[licenseId];
    u32 writeIdx = 0;

    for (u32 readIdx = 0; readIdx < lic.count; ++readIdx) {
        if (!lic.entries[readIdx].isValid) continue;
        if (IsConfirmedFriend(lic.entries[readIdx].friendCode)) {
            changed = true;
            continue;
        }
        if (writeIdx != readIdx) {
            lic.entries[writeIdx] = lic.entries[readIdx];
        }
        ++writeIdx;
    }

    for (u32 i = writeIdx; i < lic.count; ++i) {
        lic.entries[i].isValid = false;
    }
    lic.count = writeIdx;

    if (changed) {
        Save();
    }
}

void RecentPlayersStore::AddOrUpdate(u32 licenseId, const RecentPlayerEntry &entry) {
    if (licenseId >= MAX_LICENSES || !entry.isValid || entry.friendCode == 0) return;
    if (IsConfirmedFriend(entry.friendCode)) return;

    Load();
    LicenseHistory &lic = store[licenseId];

    s32 existingIdx = -1;
    for (u32 i = 0; i < lic.count; ++i) {
        if (lic.entries[i].isValid && lic.entries[i].friendCode == entry.friendCode) {
            existingIdx = static_cast<s32>(i);
            break;
        }
    }

    RecentPlayerEntry updated = entry;
    updated.encounterSeq = ++globalEncounterSeq;

    if (existingIdx >= 0) {
        for (s32 i = existingIdx; i > 0; --i) {
            lic.entries[i] = lic.entries[i - 1];
        }
        lic.entries[0] = updated;
    } else {
        u32 shiftLimit = (lic.count < MAX_ENTRIES_PER_LICENSE) ? lic.count : (MAX_ENTRIES_PER_LICENSE - 1);
        for (s32 i = static_cast<s32>(shiftLimit); i > 0; --i) {
            lic.entries[i] = lic.entries[i - 1];
        }
        lic.entries[0] = updated;
        if (lic.count < MAX_ENTRIES_PER_LICENSE) {
            lic.count++;
        }
    }
}

void FormatRelativeTime(u64 timestampTicks, wchar_t *buffer, u32 bufferSize) {
    if (!buffer || bufferSize == 0) return;
    if (timestampTicks == 0) {
        swprintf(buffer, bufferSize, L"Time unavailable");
        return;
    }

    u64 now = OS::GetTime();
    if (now < timestampTicks) {
        swprintf(buffer, bufferSize, L"Time unavailable");
        return;
    }

    u64 diff = now - timestampTicks;
    u32 sec = OS::TicksToSeconds(diff);

    if (sec < 60) {
        swprintf(buffer, bufferSize, L"Just now");
    } else if (sec < 3600) {
        u32 m = sec / 60;
        if (m == 1) {
            swprintf(buffer, bufferSize, L"1 minute ago");
        } else {
            swprintf(buffer, bufferSize, L"%u minutes ago", m);
        }
    } else if (sec < 86400) {
        u32 h = sec / 3600;
        if (h == 1) {
            swprintf(buffer, bufferSize, L"1 hour ago");
        } else {
            swprintf(buffer, bufferSize, L"%u hours ago", h);
        }
    } else if (sec < 365 * 86400) {
        u32 d = sec / 86400;
        if (d == 1) {
            swprintf(buffer, bufferSize, L"1 day ago");
        } else {
            swprintf(buffer, bufferSize, L"%u days ago", d);
        }
    } else {
        u32 y = (sec / 86400) / 365;
        if (y == 1) {
            swprintf(buffer, bufferSize, L"1 year ago");
        } else {
            swprintf(buffer, bufferSize, L"%u years ago", y);
        }
    }
}

}  // namespace RecentPlayers
}  // namespace Pulsar
