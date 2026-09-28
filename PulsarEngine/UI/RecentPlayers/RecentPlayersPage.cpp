#include <UI/RecentPlayers/RecentPlayersPage.hpp>
#include <Network/RecentPlayers/RecentPlayersStore.hpp>
#include <Network/Rating/PlayerRating.hpp>
#include <MarioKartWii/RKSYS/RKSYSMgr.hpp>
#include <MarioKartWii/Input/ControllerHolder.hpp>
#include <include/c_wchar.h>

namespace Pulsar {
namespace UI {

static void SetTextBoxIfPresent(LayoutUIControl &control, const char *paneName, u32 bmgId, const Text::Info *info) {
    if (control.layout.GetPaneByName(paneName) != nullptr) {
        control.SetTextBoxMessage(paneName, bmgId, info);
    }
}

static void SetPaneVisibleIfPresent(LayoutUIControl &control, const char *paneName, bool visible) {
    if (control.layout.GetPaneByName(paneName) != nullptr) {
        control.SetPaneVisibility(paneName, visible);
    }
}

static void SetRowTextColor(LayoutUIControl &row, const nw4r::ut::Color &textColor) {
    const char *textBoxNames[] = {"player_name", "position", "total_score", "total_point", "time"};
    for (int j = 0; j < 5; ++j) {
        nw4r::lyt::TextBox *textBox = reinterpret_cast<nw4r::lyt::TextBox *>(row.layout.GetPaneByName(textBoxNames[j]));
        if (textBox != nullptr) {
            textBox->color1[0] = textColor;
            textBox->color1[1] = textColor;
        }
    }
}

static wchar_t s_rowDash[] = L"----";
static wchar_t s_rowBlank[] = L"";

static void ClearRow(LayoutUIControl &row) {
    Text::Info dashInfo;
    dashInfo.strings[0] = s_rowDash;
    SetTextBoxIfPresent(row, "player_name", UI::BMG_TEXT, &dashInfo);

    Text::Info blankInfo;
    blankInfo.strings[0] = s_rowBlank;
    SetTextBoxIfPresent(row, "position", UI::BMG_TEXT, &blankInfo);
    SetTextBoxIfPresent(row, "total_score", UI::BMG_TEXT, &blankInfo);
    SetTextBoxIfPresent(row, "total_point", UI::BMG_TEXT, &blankInfo);
    SetTextBoxIfPresent(row, "time", UI::BMG_TEXT, &blankInfo);

    SetRowTextColor(row, nw4r::ut::Color(255, 255, 255, 255));
    SetPaneVisibleIfPresent(row, "chara_icon", false);
    SetPaneVisibleIfPresent(row, "chara_icon_sha", false);
    SetPaneVisibleIfPresent(row, "time", false);
}

RecentPlayersPage::RecentPlayersPage() : nextPageId(PAGE_NONE), curPage(0), refreshTimer(0) {
    onBackButtonClickHandler.subject = this;
    onBackButtonClickHandler.ptmf = &RecentPlayersPage::OnBackButtonClick;
    onBackPressHandler.subject = this;
    onBackPressHandler.ptmf = &RecentPlayersPage::OnBackPress;

    titleText = new CtrlMenuPageTitleText;
    bottomText = new CtrlMenuInstructionText;
    backButton = new CtrlMenuBackButton;
    for (u32 i = 0; i < kRowsPerPage; ++i) {
        rows[i] = new LayoutUIControl;
    }
    miiGroup = new MiiGroup;
    controlsManipulatorManager = new ControlsManipulatorManager;

    controlsManipulatorManager->Init(1, false);
    this->SetManipulatorManager(*controlsManipulatorManager);
    controlsManipulatorManager->SetGlobalHandler(BACK_PRESS, onBackPressHandler, false, false);
}

RecentPlayersPage::~RecentPlayersPage() {
    delete titleText;
    delete bottomText;
    delete backButton;
    for (u32 i = 0; i < kRowsPerPage; ++i) {
        delete rows[i];
    }
    delete miiGroup;
    delete controlsManipulatorManager;
}

PageId RecentPlayersPage::GetNextPage() const {
    return this->nextPageId;
}

void RecentPlayersPage::OnInit() {
    this->InitControlGroup(kRowsPerPage + 3);

    miiGroup->Init(kRowsPerPage, 0x4, nullptr);

    this->AddControl(0, *titleText, 0);
    titleText->Load(0);
    Text::Info titleInfo;
    titleInfo.strings[0] = L"Recent Players";
    titleText->SetMessage(UI::BMG_TEXT, &titleInfo);

    this->AddControl(1, *bottomText, 0);
    bottomText->Load();

    this->AddControl(2, *backButton, 0);
    backButton->Load(UI::buttonFolder, "Back", "ButtonBack", 1, 0, false);
    backButton->SetOnClickHandler(onBackButtonClickHandler, 0);

    static const char *noAnims[] = {nullptr};
    for (u32 i = 0; i < kRowsPerPage; ++i) {
        this->AddControl(3 + i, *rows[i], 0);

        ControlLoader loader(rows[i]);
        char variant[8];
        snprintf(variant, sizeof(variant), "rank%d", i + 1);
        loader.Load("result", "ResultVS", variant, noAnims);

        SetPaneVisibleIfPresent(*rows[i], "handle_text", false);
        SetPaneVisibleIfPresent(*rows[i], "time", true);
    }
}

void RecentPlayersPage::OnActivate() {
    this->nextPageId = PAGE_NONE;
    this->curPage = 0;
    this->refreshTimer = 0;

    this->PlaySound(SOUND_ID_BUTTON_SELECT, -1);
    ApplyRows();
}

void RecentPlayersPage::BeforeEntranceAnimations() {
    this->nextPageId = PAGE_NONE;
    backButton->SelectInitial(0);
}

void RecentPlayersPage::ApplyRows() {
    RKSYS::Mgr *rksys = RKSYS::Mgr::sInstance;
    u32 licenseId = (rksys && rksys->curLicenseId >= 0 && rksys->curLicenseId < 4)
                        ? static_cast<u32>(rksys->curLicenseId)
                        : 0;

    RecentPlayers::RecentPlayersStore::Get().ReconcileConfirmedFriends(licenseId);

    u32 totalCount = RecentPlayers::RecentPlayersStore::Get().GetCount(licenseId);
    u32 totalPages = (totalCount == 0) ? 1 : ((totalCount + kRowsPerPage - 1) / kRowsPerPage);
    if (curPage >= totalPages) curPage = totalPages - 1;

    u32 baseIdx = curPage * kRowsPerPage;

    for (u32 i = 0; i < kRowsPerPage; ++i) {
        u32 entryIdx = baseIdx + i;
        if (entryIdx < totalCount) {
            const RecentPlayers::RecentPlayerEntry *entry = RecentPlayers::RecentPlayersStore::Get().GetEntry(licenseId, entryIdx);
            if (entry != nullptr && entry->isValid) {
                Text::Info nameInfo;
                nameInfo.strings[0] = const_cast<wchar_t *>(entry->name);
                SetTextBoxIfPresent(*rows[i], "player_name", UI::BMG_TEXT, &nameInfo);

                wchar_t posText[8];
                swprintf(posText, sizeof(posText) / sizeof(posText[0]), L"#%u", entryIdx + 1);
                Text::Info posInfo;
                posInfo.strings[0] = posText;
                SetTextBoxIfPresent(*rows[i], "position", UI::BMG_TEXT, &posInfo);

                wchar_t ratingBuf[32];
                PointRating::FormatRatingDigits(entry->rating, ratingBuf, sizeof(ratingBuf) / sizeof(ratingBuf[0]));
                Text::Info ratingInfo;
                ratingInfo.strings[0] = ratingBuf;
                SetTextBoxIfPresent(*rows[i], "total_score", UI::BMG_TEXT, &ratingInfo);

                static wchar_t labelVR[] = L"VR";
                static wchar_t labelBR[] = L"BR";
                Text::Info labelInfo;
                labelInfo.strings[0] = entry->isBattle ? labelBR : labelVR;
                SetTextBoxIfPresent(*rows[i], "total_point", UI::BMG_TEXT, &labelInfo);

                wchar_t timeBuf[32];
                RecentPlayers::FormatRelativeTime(entry->timestampTicks, timeBuf, sizeof(timeBuf) / sizeof(timeBuf[0]));
                Text::Info timeInfo;
                timeInfo.strings[0] = timeBuf;
                SetTextBoxIfPresent(*rows[i], "time", UI::BMG_TEXT, &timeInfo);
                SetPaneVisibleIfPresent(*rows[i], "time", true);

                miiGroup->LoadMii(i, const_cast<RFL::StoreData *>(&entry->miiData));
                rows[i]->SetMiiPane("chara_icon", *miiGroup, i, 2);
                rows[i]->SetMiiPane("chara_icon_sha", *miiGroup, i, 2);
                SetPaneVisibleIfPresent(*rows[i], "chara_icon", true);
                SetPaneVisibleIfPresent(*rows[i], "chara_icon_sha", true);

                SetRowTextColor(*rows[i], nw4r::ut::Color(255, 255, 255, 255));
                continue;
            }
        }

        ClearRow(*rows[i]);
    }

    wchar_t pageText[32];
    swprintf(pageText, sizeof(pageText) / sizeof(pageText[0]), L"< %u/%u >", curPage + 1, totalPages);
    Text::Info info;
    info.strings[0] = pageText;
    bottomText->SetMessage(UI::BMG_TEXT, &info);
}

void RecentPlayersPage::ChangePage(s32 direction) {
    RKSYS::Mgr *rksys = RKSYS::Mgr::sInstance;
    u32 licenseId = (rksys && rksys->curLicenseId >= 0 && rksys->curLicenseId < 4)
                        ? static_cast<u32>(rksys->curLicenseId)
                        : 0;

    u32 totalCount = RecentPlayers::RecentPlayersStore::Get().GetCount(licenseId);
    u32 totalPages = (totalCount == 0) ? 1 : ((totalCount + kRowsPerPage - 1) / kRowsPerPage);
    if (totalPages <= 1) return;

    if (direction < 0) {
        if (curPage > 0) {
            --curPage;
            this->PlaySound(SOUND_ID_LEFT_ARROW_PRESS, -1);
            ApplyRows();
        }
    } else if (direction > 0) {
        if (curPage + 1 < totalPages) {
            ++curPage;
            this->PlaySound(SOUND_ID_RIGHT_ARROW_PRESS, -1);
            ApplyRows();
        }
    }
}

void RecentPlayersPage::OnUpdate() {
    if (++refreshTimer >= 60) {
        refreshTimer = 0;
        ApplyRows();
    }

    if (SectionMgr::sInstance == nullptr) return;

    const Input::RealControllerHolder *controllerHolder = SectionMgr::sInstance->pad.padInfos[0].controllerHolder;
    if (controllerHolder == nullptr || controllerHolder->curController == nullptr) return;

    const ControllerType controllerType = controllerHolder->curController->GetType();
    const u16 inputs = controllerHolder->inputStates[0].buttonRaw;
    const u16 newInputs = inputs & ~controllerHolder->inputStates[1].buttonRaw;

    u16 leftButton = 0;
    u16 rightButton = 0;
    if (controllerType == CLASSIC) {
        leftButton = WPAD::WPAD_CL_TRIGGER_L | WPAD::WPAD_CL_BUTTON_LEFT;
        rightButton = WPAD::WPAD_CL_TRIGGER_R | WPAD::WPAD_CL_BUTTON_RIGHT;
    } else if (controllerType == WHEEL) {
        leftButton = WPAD::WPAD_BUTTON_UP;
        rightButton = WPAD::WPAD_BUTTON_DOWN;
    } else if (controllerType == NUNCHUCK) {
        leftButton = WPAD::WPAD_BUTTON_LEFT;
        rightButton = WPAD::WPAD_BUTTON_RIGHT;
    } else {
        leftButton = PAD::PAD_BUTTON_L | PAD::PAD_BUTTON_LEFT;
        rightButton = PAD::PAD_BUTTON_R | PAD::PAD_BUTTON_RIGHT;
    }

    if ((newInputs & leftButton) != 0) {
        ChangePage(-1);
    } else if ((newInputs & rightButton) != 0) {
        ChangePage(1);
    }
}

void RecentPlayersPage::OnBackPress(u32 /*hudSlotId*/) {
    this->nextPageId = static_cast<PageId>(PULPAGE_SOCIAL);
    this->EndStateAnimated(1, 0.0f);
}

void RecentPlayersPage::OnBackButtonClick(PushButton &button, u32 /*hudSlotId*/) {
    this->nextPageId = static_cast<PageId>(PULPAGE_SOCIAL);
    this->EndStateAnimated(1, button.GetAnimationFrameSize());
}

}  // namespace UI
}  // namespace Pulsar
