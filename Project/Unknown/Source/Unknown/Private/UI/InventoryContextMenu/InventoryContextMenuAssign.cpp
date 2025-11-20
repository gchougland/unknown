#include "UI/InventoryContextMenu/InventoryContextMenuAssign.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/ProjectStyle.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/HotbarComponent.h"
#include "Player/FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Math/UnrealMathUtility.h"

namespace InventoryContextMenuActions
{
    TSharedRef<SWidget> BuildAssignSubmenu(UItemDefinition* ItemType, UInventoryScreenWidget* Widget)
    {
        const FSlateBrush* WhiteBrush = ProjectStyle::GetTerminalWhiteBrush();
        const FSlateBrush* BlackBrush = ProjectStyle::GetTerminalBlackBrush();
        const FButtonStyle* ButtonStyle = ProjectStyle::GetTerminalButtonStyle();
        const FLinearColor White = ProjectStyle::GetTerminalWhite();
        const FLinearColor Black = ProjectStyle::GetTerminalBlack();
        TWeakObjectPtr<UInventoryScreenWidget> WeakWidget(Widget);

        // Build submenu rows 1–9 with identical styling
        auto MakeSlotRow = [&](int32 SlotIndex) -> TSharedRef<SWidget>
        {
            TSharedPtr<SButton> Button;
            return SNew(SBorder)
                .BorderImage(WhiteBrush)
                .BorderBackgroundColor(White)
                .Padding(0.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SAssignNew(Button, SButton)
                        .ButtonStyle(ButtonStyle)
                        .IsFocusable(false)
                        .ContentPadding(FMargin(8.f, 4.f))
                        .ClickMethod(EButtonClickMethod::MouseDown)
                        .OnPressed_Lambda([ItemType, SlotIndex, WeakWidget]()
                        {
                            // Fire on mouse-down to beat SMenuAnchor's dismiss-on-click, ensuring our logic runs.
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign press: ItemType=%s SlotIndex(UI)=%d"), *GetNameSafe(ItemType), SlotIndex);

                            if (!WeakWidget.IsValid())
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Widget invalid (GCed?)"));
                                return;
                            }

                            APlayerController* PC = WeakWidget->GetOwningPlayer();
                            if (!PC)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Owning PlayerController is null"));
                                return;
                            }
                            APawn* Pawn = PC->GetPawn();
                            if (!Pawn)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: PC->GetPawn() returned null"));
                                return;
                            }

                            AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
                            if (!Character)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Pawn %s is not AFirstPersonCharacter"), *GetNameSafe(Pawn));
                                return;
                            }

                            UHotbarComponent* HB = Character->GetHotbar();
                            if (!HB)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Character->GetHotbar() is null"));
                                return;
                            }

                            if (!ItemType)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: ItemType is null"));
                                return;
                            }

                            const int32 ZeroBased = FMath::Clamp(SlotIndex - 1, 0, 8);
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign press: attempting AssignSlot Index=%d Type=%s"), ZeroBased, *GetNameSafe(ItemType));
                            const bool bOk = HB->AssignSlot(ZeroBased, ItemType);
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign result (press): %s for slot %d (Type=%s)"), bOk ? TEXT("Success") : TEXT("Failure"), ZeroBased, *GetNameSafe(ItemType));

                            // Close menu and restore focus to game viewport
                            FSlateApplication::Get().DismissAllMenus();
                            UWidgetBlueprintLibrary::SetFocusToGameViewport();
                            WeakWidget->SetKeyboardFocus();
                        })
                        .OnClicked_Lambda([ItemType, SlotIndex, WeakWidget]()
                        {
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign click: ItemType=%s SlotIndex(UI)=%d"), *GetNameSafe(ItemType), SlotIndex);

                            // Validate captured widget
                            if (!WeakWidget.IsValid())
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Widget invalid (GCed?)"));
                                return FReply::Handled();
                            }

                            // Resolve PC and Pawn
                            APlayerController* PC = WeakWidget->GetOwningPlayer();
                            if (!PC)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Owning PlayerController is null"));
                                return FReply::Handled();
                            }
                            APawn* Pawn = PC->GetPawn();
                            if (!Pawn)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: PC->GetPawn() returned null"));
                                return FReply::Handled();
                            }

                            // Character cast
                            AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
                            if (!Character)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Pawn %s is not AFirstPersonCharacter"), *GetNameSafe(Pawn));
                                return FReply::Handled();
                            }

                            // Hotbar resolution
                            UHotbarComponent* HB = Character->GetHotbar();
                            if (!HB)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Character->GetHotbar() is null"));
                                return FReply::Handled();
                            }

                            if (!ItemType)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: ItemType is null"));
                                return FReply::Handled();
                            }

                            const int32 ZeroBased = FMath::Clamp(SlotIndex - 1, 0, 8);
                            const bool bOk = HB->AssignSlot(ZeroBased, ItemType);

                            // Close menu and restore focus to game viewport
                            FSlateApplication::Get().DismissAllMenus();
                            UWidgetBlueprintLibrary::SetFocusToGameViewport();
                            WeakWidget->SetKeyboardFocus();
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(FString::Printf(TEXT("Slot %d"), SlotIndex)))
                            .ColorAndOpacity(White)
                        ]
                    ]
                    + SOverlay::Slot()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Fill)
                    [
                        SNew(SBorder)
                        .BorderImage(WhiteBrush)
                        .BorderBackgroundColor(White)
                        .Padding(0.f)
                        .Visibility_Lambda([Button]() -> EVisibility 
                        { 
                            return (Button.IsValid() && Button->IsHovered()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; 
                        })
                        [
                            SNew(SBox)
                            .WidthOverride(2.f)
                        ]
                    ]
                ];
        };

        TSharedRef<SVerticalBox> SubVBox = SNew(SVerticalBox);
        for (int32 SlotIndex = 1; SlotIndex <= 9; ++SlotIndex)
        {
            SubVBox->AddSlot().AutoHeight().Padding(2.f)
            [ MakeSlotRow(SlotIndex) ];
        }

        return SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(White)
            .Padding(2.f)
            [
                SNew(SBorder)
                .BorderImage(BlackBrush)
                .BorderBackgroundColor(Black)
                .Padding(6.f)
                [
                    SNew(SBox)
                    .MinDesiredWidth(160.f)
                    [ SubVBox ]
                ]
            ];
    }

    TSharedRef<SWidget> BuildAssignRow(UItemDefinition* ItemType, UInventoryScreenWidget* Widget, TSharedRef<FLocalMenuState> MenuState)
    {
        const FSlateBrush* WhiteBrush = ProjectStyle::GetTerminalWhiteBrush();
        const FButtonStyle* ButtonStyle = ProjectStyle::GetTerminalButtonStyle();
        const FLinearColor White = ProjectStyle::GetTerminalWhite();
        TWeakObjectPtr<UInventoryScreenWidget> WeakWidget(Widget);
        TWeakObjectPtr<UItemDefinition> WeakItemType(ItemType);

        TSharedPtr<SButton> OpenerButton;
        TSharedPtr<SMenuAnchor> LocalAnchor;

        TSharedRef<SWidget> Row = SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(White)
            .Padding(0.f)
            [
                SAssignNew(LocalAnchor, SMenuAnchor)
                .UseApplicationMenuStack(false) // render inline so our styling is preserved, no engine chrome
                .Method(EPopupMethod::UseCurrentWindow)
                .Placement(MenuPlacement_MenuRight)
                .OnGetMenuContent_Lambda([WeakItemType, WeakWidget]() -> TSharedRef<SWidget>
                {
                    if (UItemDefinition* Item = WeakItemType.Get())
                    {
                        if (UInventoryScreenWidget* W = WeakWidget.Get())
                        {
                            return BuildAssignSubmenu(Item, W);
                        }
                    }
                    // Return empty widget if invalid
                    return SNew(SBox);
                })
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SAssignNew(OpenerButton, SButton)
                        .ButtonStyle(ButtonStyle)
                        .IsFocusable(false)
                        .ContentPadding(FMargin(8.f, 4.f))
                        .OnHovered_Lambda([MenuState]()
                        {
                            if (TSharedPtr<SMenuAnchor> A = MenuState->AssignAnchor.Pin())
                            {
                                const bool bWasOpen = A->IsOpen();
                                if (!bWasOpen)
                                {
                                    A->SetIsOpen(true, false);
                                }
                            }
                        })
                        .OnClicked_Lambda([MenuState]()
                        {
                            if (TSharedPtr<SMenuAnchor> A = MenuState->AssignAnchor.Pin())
                            {
                                const bool bNew = !A->IsOpen();
                                A->SetIsOpen(bNew, false);
                            }
                            return FReply::Handled();
                        })
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth()
                            [ SNew(STextBlock).Text(FText::FromString(TEXT("Assign to Hotbar \u25B6"))).ColorAndOpacity(White) ]
                        ]
                    ]
                    + SOverlay::Slot()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Fill)
                    [
                        SNew(SBorder)
                        .BorderImage(WhiteBrush)
                        .BorderBackgroundColor(White)
                        .Padding(0.f)
                        .Visibility_Lambda([OpenerButton, MenuState]() -> EVisibility
                        {
                            const bool bHover = (OpenerButton.IsValid() && OpenerButton->IsHovered());
                            const bool bOpen = (MenuState->AssignAnchor.IsValid() && MenuState->AssignAnchor.Pin()->IsOpen());
                            return (bHover || bOpen) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                        })
                        [ SNew(SBox).WidthOverride(2.f) ]
                    ]
                ]
            ];

        // Share weak reference so other rows can close it on hover
        MenuState->AssignAnchor = LocalAnchor;
        return Row;
    }
}

