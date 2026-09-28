#ifndef _PUL_RECENT_PLAYERS_CAPTURE_
#define _PUL_RECENT_PLAYERS_CAPTURE_

#include <kamek.hpp>

namespace Pulsar {
namespace RecentPlayers {

void CaptureOnRaceFrame();
void FlushCapturedOpponents();
void ResetCaptureSession();

}  // namespace RecentPlayers
}  // namespace Pulsar

#endif
