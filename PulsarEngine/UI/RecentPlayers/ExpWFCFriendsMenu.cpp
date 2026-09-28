#include <UI/RecentPlayers/ExpWFCFriendsMenu.hpp>
#include <UI/RecentPlayers/RecentPlayersPage.hpp>
#include <UI/UI.hpp>

namespace Pulsar {
namespace UI {

kmWrite32(0x8064cac8, 0x60000000);  // nop InitControlGroup in WFCFriendsMenu::OnInit

ExpWFCFriendsMenu::ExpWFCFriendsMenu() {
    this->onRecentPlayersClick.subject = this;
    this->onRecentPlayersClick.ptmf = &ExpWFCFriendsMenu::OnRecentPlayersButtonClick;
    this->onExtButtonSelect.subject = this;
    this->onExtButtonSelect.ptmf = &ExpWFCFriendsMenu::ExtOnButtonSelect;
}

void ExpWFCFriendsMenu::OnInit() {
    this->InitControlGroup(7);
    WFCFriendsMenu::OnInit();

    this->AddControl(6, recentPlayersButton, 0);
    this->recentPlayersButton.Load(UI::buttonFolder, "Settings1P", "Leaderboard", 1, 0, false);
    this->recentPlayersButton.buttonId = 6;
    Text::Info info;
    info.strings[0] = L"Recent Players";
    this->recentPlayersButton.SetMessage(UI::BMG_TEXT, &info);
    this->recentPlayersButton.SetOnClickHandler(this->onRecentPlayersClick, 0);
    this->recentPlayersButton.SetOnSelectHandler(this->onExtButtonSelect);
}

void ExpWFCFriendsMenu::OnActivate() {
    this->nextPage = PAGE_NONE;
    WFCFriendsMenu::OnActivate();
}

void ExpWFCFriendsMenu::ExtOnButtonSelect(PushButton &button, u32 hudSlotId) {
    if (button.buttonId == 6) {
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
