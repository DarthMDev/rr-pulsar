#include <Network/RecentPlayers/RecentPlayersCapture.hpp>
#include <Network/RecentPlayers/RecentPlayersStore.hpp>
#include <MarioKartWii/Driver/DriverManager.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <MarioKartWii/Race/RaceData.hpp>
#include <MarioKartWii/RKNet/RKNetController.hpp>
#include <MarioKartWii/RKNet/USER.hpp>
#include <MarioKartWii/RKSYS/RKSYSMgr.hpp>
#include <MarioKartWii/UI/Section/SectionMgr.hpp>
#include <MarioKartWii/3D/Camera/CameraMgr.hpp>
#include <Network/Rating/PlayerRating.hpp>
#include <core/rvl/OS/OS.hpp>
#include <core/rvl/RFL/RFL.hpp>

namespace Pulsar {
namespace RecentPlayers {

struct CapturedOpponent {
    u64 friendCode;
    RFL::StoreData miiData;
    wchar_t name[12];
    float rating;
    bool isBattle;
    u64 timestampTicks;
    bool isValid;
};

static CapturedOpponent s_capturedBuffer[12];
static u32 s_capturedCount = 0;
static bool s_hasCapturedThisRace = false;

static void ExtractMiiName(const RFL::StoreData *storeData, wchar_t *outName, size_t outNameLen) {
    if (outName == nullptr || outNameLen == 0 || storeData == nullptr) {
        if (outName != nullptr && outNameLen > 0) outName[0] = L'\0';
        return;
    }
    size_t o = 0;
    for (int i = 0; i < 10 && o + 1 < outNameLen; ++i) {
        const u16 code = storeData->miiName[i];
        if (code == 0) break;
        outName[o++] = static_cast<wchar_t>(code);
    }
    outName[o] = L'\0';
}

void ResetCaptureSession() {
    s_capturedCount = 0;
    s_hasCapturedThisRace = false;
    for (u32 i = 0; i < 12; ++i) {
        s_capturedBuffer[i].isValid = false;
    }
}

void CaptureOnRaceFrame() {
    if (!DriverMgr::isOnlineRace) return;

    Raceinfo *raceInfo = Raceinfo::sInstance;
    // A countdown is still a lobby-to-race transition.  Only retain opponents
    // once our local console is actually in the shared race/battle.
    if (!raceInfo || raceInfo->stage < RACESTAGE_RACE) return;

    if (SectionMgr::sInstance != nullptr && SectionMgr::sInstance->curSection != nullptr) {
        SectionId secId = SectionMgr::sInstance->curSection->sectionId;
        if (secId == SECTION_P1_WIFI_VS_LIVEVIEW || secId == SECTION_P2_WIFI_VS_LIVEVIEW) return;
    }

    RaceCameraMgr *camMgr = RaceCameraMgr::sInstance;
    if (camMgr != nullptr && camMgr->isOnlineSpectating) return;

    if (s_hasCapturedThisRace) return;

    RKNet::Controller *controller = RKNet::Controller::sInstance;
    RKNet::USERHandler *userHandler = RKNet::USERHandler::sInstance;
    Racedata *raceData = Racedata::sInstance;
    if (!controller || !userHandler || !raceData) return;

    u8 localAid = controller->subs[controller->currentSub].localAid;
    u64 localFC = userHandler->toSendPacket.fc;
    const RacedataScenario &scenario = raceData->racesScenario;
    GameMode mode = scenario.settings.gamemode;
    bool isBattle = (mode == MODE_BATTLE || mode == MODE_PUBLIC_BATTLE || mode == MODE_PRIVATE_BATTLE);

    u64 now = OS::GetTime();

    for (int p = 0; p < scenario.playerCount; ++p) {
        if (scenario.players[p].playerType == PLAYER_REAL_LOCAL) continue;

        u8 aid = controller->aidsBelongingToPlayerIds[p];
        if (aid >= 12 || aid == localAid) continue;

        const RKNet::USERPacket &packet = userHandler->receivedPackets[aid];
        if (packet.fc == 0 || packet.fc == localFC) continue;

        const RFL::StoreData *storeData = &packet.rflPacket.rawMiis[0];
        if (!RFL::CheckValidRaw(storeData)) continue;

        bool alreadyCaptured = false;
        for (u32 c = 0; c < s_capturedCount; ++c) {
            if (s_capturedBuffer[c].friendCode == packet.fc) {
                alreadyCaptured = true;
                break;
            }
        }
        if (alreadyCaptured) continue;

        if (s_capturedCount < 12) {
            CapturedOpponent &opp = s_capturedBuffer[s_capturedCount++];
            opp.friendCode = packet.fc;
            opp.miiData = *storeData;
            ExtractMiiName(storeData, opp.name, sizeof(opp.name) / sizeof(opp.name[0]));
            opp.isBattle = isBattle;

            float base = static_cast<float>(isBattle ? packet.br : packet.vr);
            u8 slot = 0;
            for (int i = 0; i < p; ++i) {
                if (controller->aidsBelongingToPlayerIds[i] == aid) ++slot;
            }
            float dec = (slot < 2) ? (static_cast<float>(PointRating::remoteDecimalVR[aid][slot]) / 100.0f) : 0.0f;
            opp.rating = base + dec;
            opp.timestampTicks = now;
            opp.isValid = true;
        }
    }

    s_hasCapturedThisRace = true;
}

void FlushCapturedOpponents() {
    if (!s_hasCapturedThisRace || s_capturedCount == 0) {
        s_capturedCount = 0;
        s_hasCapturedThisRace = false;
        return;
    }

    RKSYS::Mgr *rksys = RKSYS::Mgr::sInstance;
    u32 licenseId = (rksys && rksys->curLicenseId >= 0 && rksys->curLicenseId < static_cast<s32>(MAX_LICENSES))
                        ? static_cast<u32>(rksys->curLicenseId)
                        : 0;

    for (u32 i = 0; i < s_capturedCount; ++i) {
        if (!s_capturedBuffer[i].isValid) continue;

        RecentPlayerEntry entry;
        entry.friendCode = s_capturedBuffer[i].friendCode;
        entry.miiData = s_capturedBuffer[i].miiData;
        for (int k = 0; k < 12; ++k) entry.name[k] = s_capturedBuffer[i].name[k];
        entry.rating = s_capturedBuffer[i].rating;
        entry.isBattle = s_capturedBuffer[i].isBattle;
        entry.timestampTicks = s_capturedBuffer[i].timestampTicks;
        entry.encounterSeq = 0;
        entry.isValid = true;

        RecentPlayersStore::Get().AddOrUpdate(licenseId, entry);
    }

    RecentPlayersStore::Get().Save();

    s_capturedCount = 0;
    s_hasCapturedThisRace = false;
}

// These project-wide hooks are deliberately used instead of changing the
// RaceScene vtable.  The latter runs while core scene state is still being
// assembled and was the source of the startup crash at 0x80554208.
static RaceLoadHook ResetRecentPlayersCaptureOnRaceLoad(ResetCaptureSession);
static SectionLoadHook FlushRecentPlayersCaptureOnSectionLoad(FlushCapturedOpponents);

}  // namespace RecentPlayers
}  // namespace Pulsar
