#include "Icons/ItemIconSettings.h"

UItemIconSettings::UItemIconSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

const UItemIconSettings* UItemIconSettings::Get()
{
    return GetDefault<UItemIconSettings>();
}
