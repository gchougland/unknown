#include "Player/FirstPersonPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Player/FirstPersonCharacter.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Inventory/HotbarComponent.h"
#include "UI/InteractHighlightWidget.h"
#include "UI/InteractInfoWidget.h"
#include "UI/HotbarWidget.h"
#include "UI/DropProgressBarWidget.h"
#include "UI/StatBarWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "Player/HungerComponent.h"
#include "UI/StorageWindowWidget.h"
#include "UI/StorageListWidget.h"
#include "UI/PauseMenuWidget.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// Helper struct for UI management logic
struct FPlayerControllerUIManager
{
	// Initialize all widgets (called from BeginPlay)
	static void InitializeWidgets(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		// Create and add the interact highlight widget (full-screen overlay)
		if (!PC->InteractHighlightWidget)
		{
			PC->InteractHighlightWidget = CreateWidget<UInteractHighlightWidget>(PC, UInteractHighlightWidget::StaticClass());
			if (PC->InteractHighlightWidget)
			{
				PC->InteractHighlightWidget->AddToViewport(1000); // high Z-order to render above HUD
				// Fill the entire viewport so our paint coordinates map 1:1 to screen space
				PC->InteractHighlightWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
				PC->InteractHighlightWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				// Keep widget always present but non-blocking; use SetVisible() only for the highlight border
				PC->InteractHighlightWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				PC->InteractHighlightWidget->SetVisible(false);
			}
		}

		// Create and add the interact info widget (full-screen overlay)
		if (!PC->InteractInfoWidget)
		{
			PC->InteractInfoWidget = CreateWidget<UInteractInfoWidget>(PC, UInteractInfoWidget::StaticClass());
			if (PC->InteractInfoWidget)
			{
				PC->InteractInfoWidget->AddToViewport(1001); // slightly higher Z-order than highlight
				// Fill the entire viewport so our paint coordinates map 1:1 to screen space
				PC->InteractInfoWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
				PC->InteractInfoWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				PC->InteractInfoWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				PC->InteractInfoWidget->SetVisible(false);
			}
		}

		// Bind to physics interaction state to hide highlight while holding
		if (AFirstPersonCharacter* C0 = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			if (UPhysicsInteractionComponent* PIC0 = C0->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				TWeakObjectPtr<AFirstPersonPlayerController> WeakPC(PC);
				PIC0->OnPickedUp.AddLambda([WeakPC](UPrimitiveComponent* /*Comp*/, const FVector& /*Pivot*/)
				{
					if (WeakPC.IsValid() && WeakPC->InteractHighlightWidget)
					{
						WeakPC->InteractHighlightWidget->SetVisible(false);
					}
				});
				// On release/throw we don't force-show; next tick will recompute normally
				PIC0->OnReleased.AddLambda([](UPrimitiveComponent* /*Comp*/)
				{
					// no-op
				});
				PIC0->OnThrown.AddLambda([](UPrimitiveComponent* /*Comp*/, const FVector& /*Dir*/)
				{
					// no-op
				});
			}
		}

		// Create and add the hotbar widget (always visible, left side)
		if (!PC->HotbarWidget)
		{
			PC->HotbarWidget = CreateWidget<UHotbarWidget>(PC, UHotbarWidget::StaticClass());
			if (PC->HotbarWidget)
			{
				PC->HotbarWidget->SetOwningPlayer(PC);
				PC->HotbarWidget->AddToPlayerScreen(500);
				PC->HotbarWidget->SetVisibility(ESlateVisibility::Visible);
				PC->HotbarWidget->SetRenderOpacity(1.f);
				PC->HotbarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.1f, 0.f, 0.9f));
				PC->HotbarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				// No padding between the left edge of the screen and the slots
				PC->HotbarWidget->SetPositionInViewport(FVector2D(0.f, 50.f), false);
				PC->HotbarWidget->SetDesiredSizeInViewport(FVector2D(120.f, 9.f * 72.f));

				if (AFirstPersonCharacter* C1 = Cast<AFirstPersonCharacter>(PC->GetPawn()))
				{
					if (UHotbarComponent* HB = C1->GetHotbar())
					{
						PC->HotbarWidget->SetHotbar(HB);
					}
				}
			}
		}

		// Create and add the drop progress bar widget (full-screen overlay)
		if (!PC->DropProgressBarWidget)
		{
			PC->DropProgressBarWidget = CreateWidget<UDropProgressBarWidget>(PC, UDropProgressBarWidget::StaticClass());
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->AddToViewport(1001); // higher Z-order than highlight widget
				PC->DropProgressBarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
				PC->DropProgressBarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				PC->DropProgressBarWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				PC->DropProgressBarWidget->SetVisible(false);
			}
		}

		// Create and add the hunger bar widget (below hotbar, left side)
		if (!PC->HungerBarWidget)
		{
			PC->HungerBarWidget = CreateWidget<UStatBarWidget>(PC, UStatBarWidget::StaticClass());
			if (PC->HungerBarWidget)
			{
				PC->HungerBarWidget->SetOwningPlayer(PC);
				PC->HungerBarWidget->AddToPlayerScreen(500);
				PC->HungerBarWidget->SetVisibility(ESlateVisibility::Visible);
				PC->HungerBarWidget->SetRenderOpacity(1.f);
				PC->HungerBarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f));
				PC->HungerBarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				// Position below hotbar: hotbar is at Y=50 with height 648, so hunger bar at Y=50+648+20=718
				PC->HungerBarWidget->SetPositionInViewport(FVector2D(0.f, 718.f), false);
				PC->HungerBarWidget->SetDesiredSizeInViewport(FVector2D(120.f, 24.f)); // Width matches hotbar, height for taller bar
				
				// Configure hunger bar appearance
				PC->HungerBarWidget->SetLabel(TEXT("HGR"));
				PC->HungerBarWidget->SetFillColor(FLinearColor(0.8f, 0.2f, 0.2f, 1.0f)); // Red
			}
		}

		// Create and add the pause menu widget
		if (!PC->PauseMenuWidget)
		{
			PC->PauseMenuWidget = CreateWidget<UPauseMenuWidget>(PC, UPauseMenuWidget::StaticClass());
			if (PC->PauseMenuWidget)
			{
				PC->PauseMenuWidget->AddToViewport(2000); // High Z-order to appear above everything
				PC->PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
				
				// Center on screen - MUST be set AFTER AddToViewport
				// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
				// Increased height to accommodate all buttons with proper spacing
				PC->PauseMenuWidget->SetDesiredSizeInViewport(FVector2D(300.f, 500.f));
				PC->PauseMenuWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
				PC->PauseMenuWidget->SetPositionInViewport(FVector2D(0.f, 0.f)); // Center - position 0,0 with center anchor/alignment centers the widget
				// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
				PC->PauseMenuWidget->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			}
		}
	}

	// Toggle inventory screen
	static void ToggleInventory(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		// Lazy-create inventory widget
		if (!PC->InventoryScreen)
		{
			PC->InventoryScreen = CreateWidget<UInventoryScreenWidget>(PC, UInventoryScreenWidget::StaticClass());
			if (PC->InventoryScreen)
			{
				PC->InventoryScreen->AddToViewport(900);
				PC->InventoryScreen->SetAnchorsInViewport(FAnchors(0.2f, 0.2f, 0.8f, 0.8f));
				PC->InventoryScreen->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				// Close the inventory when the widget requests it (Tab/I/Esc)
				PC->InventoryScreen->OnRequestClose.AddDynamic(PC, &AFirstPersonPlayerController::ToggleInventory);
				UE_LOG(LogTemp, Display, TEXT("[PC] Inventory widget created and added to viewport"));
			}
		}
		else
		{
			PC->InventoryScreen->SetAnchorsInViewport(FAnchors(0.2f, 0.2f, 0.8f, 0.8f));
		}

		if (!PC->InventoryScreen)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] ToggleInventory: InventoryScreen is null (failed to create)"));
			return;
		}

		const bool bCurrentlyOpen = PC->InventoryScreen->IsOpen();
		if (bCurrentlyOpen)
		{
			PC->bInventoryUIOpen = false;
			PC->InventoryScreen->Close();
			
			// Close storage window if it's open
			if (PC->StorageWindow && PC->StorageWindow->IsOpen())
			{
				PC->StorageWindow->Close();
				PC->StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
			}
			
			// Restore crosshair/highlight when closing inventory
			if (PC->InteractHighlightWidget)
			{
				PC->InteractHighlightWidget->SetCrosshairEnabled(true);
				PC->InteractHighlightWidget->SetVisible(false); // will auto-show next tick if a target exists
			}
			FInputModeGameOnly GM;
			PC->SetInputMode(GM);
			PC->SetIgnoreLookInput(false);
			PC->SetIgnoreMoveInput(false);
			PC->bShowMouseCursor = false;
			UE_LOG(LogTemp, Display, TEXT("[PC] Inventory closed; returning to GameOnly input"));
		}
		else
		{
			UInventoryComponent* Inv = nullptr;
			if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(PC->GetPawn()))
			{
				Inv = Char->GetInventory();
			}
			PC->InventoryScreen->Open(Inv, nullptr);
			PC->bInventoryUIOpen = true;
			// Hide crosshair/highlight behind the inventory
			if (PC->InteractHighlightWidget)
			{
				PC->InteractHighlightWidget->SetCrosshairEnabled(false);
				PC->InteractHighlightWidget->SetVisible(false);
			}
			// Switch to UI-only input and ignore game look/move
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetIgnoreLookInput(true);
			PC->SetIgnoreMoveInput(true);
			PC->bShowMouseCursor = true;
			UE_LOG(LogTemp, Display, TEXT("[PC] Inventory opened; using UIOnly input"));
		}
	}

	// Open inventory with storage
	static void OpenInventoryWithStorage(AFirstPersonPlayerController* PC, UStorageComponent* StorageComp)
	{
		if (!PC)
		{
			return;
		}

		// Lazy-create inventory widget if needed
		if (!PC->InventoryScreen)
		{
			PC->InventoryScreen = CreateWidget<UInventoryScreenWidget>(PC, UInventoryScreenWidget::StaticClass());
			if (PC->InventoryScreen)
			{
				// Position inventory screen to the right (60% of screen, starting at 40%)
				PC->InventoryScreen->AddToViewport(900);
				PC->InventoryScreen->SetAnchorsInViewport(FAnchors(0.4f, 0.2f, 0.9f, 0.8f));
				PC->InventoryScreen->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				// Close the inventory when the widget requests it (Tab/I/Esc)
				PC->InventoryScreen->OnRequestClose.AddDynamic(PC, &AFirstPersonPlayerController::ToggleInventory);
				UE_LOG(LogTemp, Display, TEXT("[PC] Inventory widget created and added to viewport"));
			}
		}
		else
		{
			PC->InventoryScreen->SetAnchorsInViewport(FAnchors(0.4f, 0.2f, 0.9f, 0.8f));
		}

		if (!PC->InventoryScreen)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] OpenInventoryWithStorage: InventoryScreen is null (failed to create)"));
			return;
		}

		// Get player inventory
		UInventoryComponent* PlayerInv = nullptr;
		if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			PlayerInv = Char->GetInventory();
		}

		// Open or update inventory screen
		const bool bCurrentlyOpen = PC->InventoryScreen->IsOpen();
		if (!bCurrentlyOpen)
		{
			// Open inventory with storage reference (so left-click transfer works)
			PC->InventoryScreen->Open(PlayerInv, StorageComp);
			PC->bInventoryUIOpen = true;
			
			// Hide crosshair/highlight behind the inventory
			if (PC->InteractHighlightWidget)
			{
				PC->InteractHighlightWidget->SetCrosshairEnabled(false);
				PC->InteractHighlightWidget->SetVisible(false);
			}
			
			// Switch to UI-only input and ignore game look/move
			FInputModeUIOnly Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetIgnoreLookInput(true);
			PC->SetIgnoreMoveInput(true);
			PC->bShowMouseCursor = true;
			UE_LOG(LogTemp, Display, TEXT("[PC] Inventory opened with storage; using UIOnly input"));
		}
		else
		{
			// Already open - just refresh
			PC->InventoryScreen->Refresh();
			UE_LOG(LogTemp, Display, TEXT("[PC] Updated open inventory with storage"));
		}

		// Show and open storage window separately (to the left of inventory)
		if (StorageComp)
		{
			UStorageWindowWidget* StorageWin = GetStorageWindow(PC);
			if (StorageWin)
			{
				// Position storage window to the left (40% of screen, starting at 5%)
				StorageWin->SetAnchorsInViewport(FAnchors(0.05f, 0.2f, 0.4f, 0.8f));
				StorageWin->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				
				// Get ItemDefinition from the storage component's owner (should be an AItemPickup)
				UItemDefinition* ContainerDef = nullptr;
				if (AActor* StorageOwner = StorageComp->GetOwner())
				{
					if (AItemPickup* Pickup = Cast<AItemPickup>(StorageOwner))
					{
						ContainerDef = Pickup->GetItemDef();
					}
				}
				
				// Open the storage window
				StorageWin->Open(StorageComp, ContainerDef);
				StorageWin->SetVisibility(ESlateVisibility::Visible);
				
				// Bind storage list left-click handler
				if (UStorageListWidget* StorageList = StorageWin->GetStorageList())
				{
					StorageList->OnItemLeftClicked.RemoveAll(PC->InventoryScreen);
					StorageList->OnItemLeftClicked.AddDynamic(PC->InventoryScreen, &UInventoryScreenWidget::HandleStorageItemLeftClick);
					UE_LOG(LogTemp, Display, TEXT("[PC] Storage window opened and StorageList bound"));
				}
				
				// Storage changed delegates are bound in StorageWindowWidget::Open()
			}
		}
		else
		{
			// Hide storage window if no storage
			if (PC->StorageWindow)
			{
				PC->StorageWindow->Close();
				PC->StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// Get or create storage window widget
	static UStorageWindowWidget* GetStorageWindow(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return nullptr;
		}

		// Lazy-create storage window widget
		if (!PC->StorageWindow)
		{
			PC->StorageWindow = CreateWidget<UStorageWindowWidget>(PC, UStorageWindowWidget::StaticClass());
			if (PC->StorageWindow)
			{
				PC->StorageWindow->AddToViewport(901); // Higher z-order than inventory (900)
				// Position to the left of inventory screen (40% width, starting at 5%)
				PC->StorageWindow->SetAnchorsInViewport(FAnchors(0.05f, 0.2f, 0.4f, 0.8f));
				PC->StorageWindow->SetAlignmentInViewport(FVector2D(0.f, 0.f));
				PC->StorageWindow->SetVisibility(ESlateVisibility::Collapsed); // Hidden by default
				UE_LOG(LogTemp, Display, TEXT("[PC] Storage window widget created and added to viewport"));
			}
		}
		return PC->StorageWindow;
	}
};

