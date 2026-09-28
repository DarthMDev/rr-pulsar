#ifndef _PUL_SOCIAL_PAGE_
#define _PUL_SOCIAL_PAGE_

#include <kamek.hpp>
#include <MarioKartWii/UI/Page/Page.hpp>
#include <MarioKartWii/UI/Ctrl/Menu/CtrlMenuText.hpp>
#include <MarioKartWii/UI/Ctrl/UIControl.hpp>
#include <UI/UI.hpp>

namespace Pulsar {
namespace UI {

class SocialPage : public Page {
public:
    static const PulPageId id = PULPAGE_SOCIAL;

    SocialPage();
    ~SocialPage() override;
    PageId GetNextPage() const override;
    void OnInit() override;
    void OnActivate() override;
    void BeforeEntranceAnimations() override;

private:
    void OnRecentPlayersClick(PushButton &button, u32 hudSlotId);
    void OnVRLeaderboardClick(PushButton &button, u32 hudSlotId);
    void OnBackButtonClick(PushButton &button, u32 hudSlotId);
    void OnBackPress(u32 hudSlotId);
    void OnButtonSelect(PushButton &button, u32 hudSlotId);

    CtrlMenuPageTitleText *titleText;
    CtrlMenuInstructionText *bottomText;
    CtrlMenuBackButton *backButton;
    PushButton *recentPlayersButton;
    PushButton *vrLeaderboardButton;
    ControlsManipulatorManager *controlsManipulatorManager;

    PtmfHolder_2A<SocialPage, void, PushButton &, u32> onRecentPlayersClickHandler;
    PtmfHolder_2A<SocialPage, void, PushButton &, u32> onVRLeaderboardClickHandler;
    PtmfHolder_2A<SocialPage, void, PushButton &, u32> onBackButtonClickHandler;
    PtmfHolder_1A<SocialPage, void, u32> onBackPressHandler;
    PtmfHolder_2A<SocialPage, void, PushButton &, u32> onButtonSelectHandler;
    PageId nextPageId;
};

}  // namespace UI
}  // namespace Pulsar

#endif
