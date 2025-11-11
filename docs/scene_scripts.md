# Scene Scripts

To create a scene script, place a `.script` file in the same folder as a scene
blend file with the same name but replace the extension `.blend` with `.script`.

## Room update function

If you define a function with the same name as a room, that function will
be called on loop while that room is active. Keep in mind any blocking
actions will pause the execution of the script and prevent other
actions from running in the meantime.

## Entity spawners

If you define a variable with the same name as an entity in the scene with the added
suffix `_spawner` and give it the type `entity_spawner` it will allow the scene
script to dynamically spawn an entity. A spawner can only have a single active
entity at a time. If you don't want to entity to spawn on load but wait for the script
to spawn it you can simply set the condition for the entity to be `false`.