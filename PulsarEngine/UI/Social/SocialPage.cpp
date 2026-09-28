#include <UI/Social/SocialPage.hpp>

namespace Pulsar {
namespace UI {

SocialPage::SocialPage() : nextPageId(PAGE_NONE) {
    onRecentPlayersClickHandler.subject = this;
    onRecentPlayersClickHandler.ptmf = &SocialPage::OnRecentPlayersClick;
    onVRLeaderboardClickHandler.subject = this;
    onVRLeaderboardClickHandler.ptmf = &SocialPage::OnVRLeaderboardClick;
    onBackButtonClickHandler.subject = this;
    onBackButtonClickHandler.ptmf = &SocialPage::OnBackButtonClick;
    onBackPressHandler.subject = this;
    onBackPressHandler.ptmf = &SocialPage::OnBackPress;
    onButtonSelectHandler.subject = this;
    onButtonSelectHandler.ptmf = &SocialPage::OnButtonSelect;

    titleText = new CtrlMenuPageTitleText;
    bottomText = new CtrlMenuInstructionText;
    backButton = new CtrlMenuBackButton;
    recentPlayersButton = new PushButton;
    vrLeaderboardButton = new PushButton;
    controlsManipulatorManager = new ControlsManipulatorManager;
    controlsManipulatorManager->Init(1, false);
    this->SetManipulatorManager(*controlsManipulatorManager);
    controlsManipulatorManager->SetGlobalHandler(BACK_PRESS, onBackPressHandler, false, false);
}

SocialPage::~SocialPage() {
    delete titleText;
    delete bottomText;
    delete backButton;
    delete recentPlayersButton;
    delete vrLeaderboardButton;
    delete controlsManipulatorManager;
}

PageId SocialPage::GetNextPage() const {
    return nextPageId;
}

void SocialPage::OnInit() {
    this->InitControlGroup(5);

    this->AddControl(0, *titleText, 0);
    titleText->Load(0);
    Text::Info title;
    title.strings[0] = L"Social";
    titleText->SetMessage(UI::BMG_TEXT, &title);

    this->AddControl(1, *bottomText, 0);
    bottomText->Load();

    this->AddControl(2, *backButton, 0);
    backButton->Load(UI::buttonFolder, "Back", "ButtonBack", 1, 0, false);
    backButton->SetOnClickHandler(onBackButtonClickHandler, 0);

    this->AddControl(3, *recentPlayersButton, 0);
    recentPlayersButton->Load(UI::buttonFolder, "WifiFriendMenu", "ButtonRegister", 1, 0, false);
    recentPlayersButton->buttonId = 0;
    Text::Info recentPlayers;
    recentPlayers.strings[0] = L"Recent Players";
    recentPlayersButton->SetMessage(UI::BMG_TEXT, &recentPlayers);
    recentPlayersButton->SetOnClickHandler(onRecentPlayersClickHandler, 0);
    recentPlayersButton->SetOnSelectHandler(onButtonSelectHandler);

    this->AddControl(4, *vrLeaderboardButton, 0);
    vrLeaderboardButton->Load(UI::buttonFolder, "WifiFriendMenu", "ButtonRegister", 1, 0, false);
    vrLeaderboardButton->buttonId = 1;
    Text::Info leaderboard;
    leaderboard.strings[0] = L"VR Leaderboard";
    vrLeaderboardButton->SetMessage(UI::BMG_TEXT, &leaderboard);
    vrLeaderboardButton->SetOnClickHandler(onVRLeaderboardClickHandler, 0);
    vrLeaderboardButton->SetOnSelectHandler(onButtonSelectHandler);

    // Both controls use the same resource, so these coordinates yield two
    // centred, evenly spaced rows without modifying an SZS layout asset.
    for (u32 layoutIdx = 0; layoutIdx < 4; ++layoutIdx) {
        recentPlayersButton->positionAndscale[layoutIdx].position.y = 36.0f;
        vrLeaderboardButton->positionAndscale[layoutIdx].position.y = -5.0f;
    }
}

void SocialPage::OnActivate() {
    nextPageId = PAGE_NONE;
}

void SocialPage::BeforeEntranceAnimations() {
    nextPageId = PAGE_NONE;
    recentPlayersButton->SelectInitial(0);
}

void SocialPage::OnRecentPlayersClick(PushButton &button, u32 /*hudSlotId*/) {
    nextPageId = static_cast<PageId>(PULPAGE_RECENT_PLAYERS);
    EndStateAnimated(0, button.GetAnimationFrameSize());
}

void SocialPage::OnVRLeaderboardClick(PushButton &button, u32 /*hudSlotId*/) {
    nextPageId = static_cast<PageId>(PULPAGE_VRLEADERBOARD);
    EndStateAnimated(0, button.GetAnimationFrameSize());
}

void SocialPage::OnBackButtonClick(PushButton &button, u32 /*hudSlotId*/) {
    nextPageId = PAGE_WFC_MAIN;
    EndStateAnimated(1, button.GetAnimationFrameSize());
}

void SocialPage::OnBackPress(u32 /*hudSlotId*/) {
    nextPageId = PAGE_WFC_MAIN;
    EndStateAnimated(1, 0.0f);
}

void SocialPage::OnButtonSelect(PushButton &button, u32 /*hudSlotId*/) {
    static wchar_t recentHelp[] = L"View players from recent races.";
    static wchar_t leaderboardHelp[] = L"View worldwide VR rankings.";
    Text::Info info;
    info.strings[0] = button.buttonId == 0 ? recentHelp : leaderboardHelp;
    bottomText->SetMessage(UI::BMG_TEXT, &info);
}

}  // namespace UI
}  // namespace Pulsar
