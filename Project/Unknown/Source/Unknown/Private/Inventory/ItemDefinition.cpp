#include "Inventory/ItemDefinition.h"

UItemDefinition::UItemDefinition()
{
    if (!Guid.IsValid())
    {
        Guid = FGuid::NewGuid();
    }
    // Initialize default transforms to identity for safe authoring defaults
    DefaultHoldTransform = FTransform::Identity;
    DefaultDropTransform = FTransform::Identity;
    IconCaptureTransform = FTransform::Identity;
}