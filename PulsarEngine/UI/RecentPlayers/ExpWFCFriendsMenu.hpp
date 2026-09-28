#ifndef _PUL_EXP_WFC_FRIENDS_MENU_
#define _PUL_EXP_WFC_FRIENDS_MENU_

#include <kamek.hpp>
#include <MarioKartWii/UI/Page/Other/WFCMenu.hpp>

namespace Pulsar {
namespace UI {

class ExpWFCFriendsMenu : public Pages::WFCFriendsMenu {
public:
    ExpWFCFriendsMenu();
    void OnInit() override;
    void OnActivate() override;
    void ExtOnButtonSelect(PushButton &button, u32 hudSlotId);
    void OnRecentPlayersButtonClick(PushButton &button, u32 hudSlotId);

    PushButton recentPlayersButton;
    PtmfHolder_2A<ExpWFCFriendsMenu, void, PushButton &, u32> onRecentPlayersClick;
    PtmfHolder_2A<ExpWFCFriendsMenu, void, PushButton &, u32> onExtButtonSelect;
};

}  // namespace UI
}  // namespace Pulsar

#endif
