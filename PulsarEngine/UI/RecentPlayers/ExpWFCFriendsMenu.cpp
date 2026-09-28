#include <UI/RecentPlayers/ExpWFCFriendsMenu.hpp>
#include <UI/RecentPlayers/RecentPlayersPage.hpp>
#include <UI/UI.hpp>

namespace Pulsar {
namespace UI {

ExpWFCFriendsMenu::ExpWFCFriendsMenu() {
    this->onRecentPlayersClick.subject = this;
    this->onRecentPlayersClick.ptmf = &ExpWFCFriendsMenu::OnRecentPlayersButtonClick;
    this->onExtButtonSelect.subject = this;
    this->onExtButtonSelect.ptmf = &ExpWFCFriendsMenu::ExtOnButtonSelect;
}

void ExpWFCFriendsMenu::OnInit() {
    // Let the game fully initialise the Friends menu first.  It owns Create
    // Room visibility, focus order, sounds, and the Back handler.
    WFCFriendsMenu::OnInit();

    // The base page creates six controls.  Rebind those loaded controls into
    // a seven-slot group before appending Recent Players.
    this->InitControlGroup(7);
    this->AddControl(0, this->titleText, 0);
    this->AddControl(1, this->rosterButton, 0);
    this->AddControl(2, this->registerButton, 0);
    this->AddControl(3, this->createRoomButton, 0);
    this->AddControl(4, this->backButton, 0);
    this->AddControl(5, this->bottomText, 0);

    this->AddControl(6, recentPlayersButton, 0);
    this->recentPlayersButton.Load(UI::buttonFolder, "WifiFriendMenu", "ButtonRegister", 1, 0, false);
    this->recentPlayersButton.buttonId = 3;
    Text::Info info;
    info.strings[0] = L"Recent Players";
    this->recentPlayersButton.SetMessage(UI::BMG_TEXT, &info);
    this->recentPlayersButton.SetOnClickHandler(this->onRecentPlayersClick, 0);
    this->recentPlayersButton.SetOnSelectHandler(this->onExtButtonSelect);

    // The stock Friends layout only reserves three full-width slots.  Make
    // room for a fourth by shifting the complete action stack upward, rather
    // than drawing over the owner Mii and friend code underneath it.
    //
    // These are screen-layout coordinates (not emulator-only pixels), so the
    // exact same layout is used by the Wii UI renderer.
    for (u32 layoutIdx = 0; layoutIdx < 4; ++layoutIdx) {
        // Each stock control has a different authored offset in its BRCTR,
        // so these values are deliberately not evenly spaced coordinates.
        // Their rendered centres are a compact, evenly spaced four-row stack.
        this->createRoomButton.positionAndscale[layoutIdx].position.y = 80.0f;
        this->rosterButton.positionAndscale[layoutIdx].position.y = 49.0f;
        this->registerButton.positionAndscale[layoutIdx].position.y = 19.0f;
        this->recentPlayersButton.positionAndscale[layoutIdx].position.y = -10.0f;
    }
}

void ExpWFCFriendsMenu::OnActivate() {
    this->nextPage = PAGE_NONE;
    WFCFriendsMenu::OnActivate();
}

void ExpWFCFriendsMenu::ExtOnButtonSelect(PushButton &button, u32 hudSlotId) {
    if (button.buttonId == 3) {
        Text::Info info;
        info.strings[0] = L"View players from recent races.";
        this->bottomText.SetMessage(UI::BMG_TEXT, &info);
    } else {
        this->OnButtonSelect(button, hudSlotId);
    }
}

void ExpWFCFriendsMenu::OnRecentPlayersButtonClick(PushButton &button, u32 hudSlotId) {
    this->nextPage = static_cast<PageId>(PULPAGE_RECENT_PLAYERS);
    this->EndStateAnimated(0, button.GetAnimationFrameSize());
}

}  // namespace UI
}  // namespace Pulsar
