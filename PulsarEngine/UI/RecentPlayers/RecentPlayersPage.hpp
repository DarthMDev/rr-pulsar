#ifndef _PUL_RECENT_PLAYERS_PAGE_
#define _PUL_RECENT_PLAYERS_PAGE_

#include <kamek.hpp>
#include <MarioKartWii/UI/Page/Page.hpp>
#include <MarioKartWii/UI/Ctrl/Menu/CtrlMenuText.hpp>
#include <MarioKartWii/UI/Ctrl/UIControl.hpp>
#include <MarioKartWii/Mii/MiiGroup.hpp>
#include <UI/UI.hpp>

namespace Pulsar {
namespace UI {

class RecentPlayersPage : public Page {
public:
    static const PulPageId id = PULPAGE_RECENT_PLAYERS;
    static const u32 kRowsPerPage = 5;

    RecentPlayersPage();
    ~RecentPlayersPage() override;

    PageId GetNextPage() const override;
    void OnInit() override;
    void OnActivate() override;
    void BeforeEntranceAnimations() override;
    void OnUpdate() override;

private:
    void OnBackPress(u32 hudSlotId);
    void OnBackButtonClick(PushButton &button, u32 hudSlotId);
    void ApplyRows();
    void ChangePage(s32 direction);

    CtrlMenuPageTitleText *titleText;
    CtrlMenuInstructionText *bottomText;
    CtrlMenuBackButton *backButton;
    LayoutUIControl *rows[kRowsPerPage];
    MiiGroup *miiGroup;

    ControlsManipulatorManager *controlsManipulatorManager;
    PtmfHolder_2A<RecentPlayersPage, void, PushButton &, u32> onBackButtonClickHandler;
    PtmfHolder_1A<RecentPlayersPage, void, u32> onBackPressHandler;

    PageId nextPageId;
    u32 curPage;
    u32 refreshTimer;
};

}  // namespace UI
}  // namespace Pulsar

#endif
