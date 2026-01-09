# Save System Usage Guide

## Overview

The save system automatically handles most game state, but some actors require manual setup of the `SaveableActorComponent` to be saved.

## Automatic Setup

The following actors **automatically** have `SaveableActorComponent` added:

1. **ItemPickup actors** - All item pickups (spawned via Item Placer tool, dropped by player, etc.) automatically get the component in their constructor.

## Manual Setup Required

You need to manually add `SaveableActorComponent` to:

### 1. Storage Containers
Any actor with a `UStorageComponent` that you want to save its inventory contents.

**How to add:**
- In the Blueprint editor, add `SaveableActorComponent` to the actor
- The component will automatically generate a unique GUID for each placed instance
- The save system will automatically save/restore the container's inventory

**Note:** The save system will attempt to add the component automatically if missing, but it's better to add it in the Blueprint for proper GUID generation.

### 2. Physics Objects (Movable Props)
Any physics-enabled actors that players can move/interact with and you want their positions saved.

**How to add:**
- In the Blueprint editor, add `SaveableActorComponent` to the actor
- Ensure the actor has physics simulation enabled (`SetSimulatePhysics(true)`)
- Set `bSaveTransform = true` (default)
- Set `bSavePhysicsState = true` if you want to save velocity (default)

**What gets saved:**
- Only physics objects that have moved significantly from their original spawn/placed position
- Threshold: 1cm position change or 1 degree rotation change
- Transform, linear velocity, and angular velocity

### 3. Other World Objects
Any other actors in the world that need to be saved (doors, switches, quest objects, etc.).

**How to add:**
- Add `SaveableActorComponent` in the Blueprint
- Configure `bSaveTransform` as needed
- The component will track the actor's state for saving

## Blueprint Instance GUIDs

**Important:** When you add `SaveableActorComponent` to a Blueprint:

- Each **placed instance** in the level will automatically get a **unique GUID** when the component is initialized
- The GUID is generated in `PostInitProperties()`, ensuring each instance is unique
- You should **NOT** set a default GUID in the Blueprint - leave it empty/invalid
- The component will generate a new GUID for each instance automatically

## Component Properties

- **PersistentId**: Unique identifier (auto-generated, don't set manually)
- **OriginalTransform**: Original spawn/placed transform (auto-captured)
- **bSaveTransform**: Whether to save this actor's transform (default: true)
- **bSavePhysicsState**: Whether to save physics velocity (default: true)

## Save System Behavior

The save system will:

1. **Automatically save:**
   - All ItemPickup actors (they have the component by default)
   - All actors with `SaveableActorComponent` where `bSaveTransform == true`
   - Only physics objects that have moved from their original position

2. **Automatically restore:**
   - Actor transforms by GUID lookup
   - Physics velocities if `bSavePhysicsState == true`
   - Container inventories by GUID lookup

3. **Handle missing actors:**
   - If an actor with a saved GUID is not found (destroyed/removed), a warning is logged
   - The save system continues loading other actors

## Best Practices

1. **Add SaveableActorComponent in Blueprints** for actors that need saving
2. **Don't set default GUIDs** in Blueprints - let the component generate them
3. **Test save/load** after placing new actors to ensure they're being saved
4. **Use descriptive actor names** to help identify actors in save logs


