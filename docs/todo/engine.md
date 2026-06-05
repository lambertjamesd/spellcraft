
* Material rendering
* Particle group improvements
* Scene fog
* Object modifiers being applied on export
* Ignore collection in blender
* Move action indicator float over target instead of in the UI

Details

[3]
I saw you were doing a material system refactor? Having control of all the options would be great including all the stuff like IM_RD and the coverage stuff etc (in the future I would like to experiment with doing AA on the characters only and not the environment, kinda like Rare did). Or the texture filter type, because I'm sure being able to have a material with an unfiltered texture will come in handy at some point. And the four-sample filter might be handy for some menu stuff. 
The only things I'm pretty sure I won't need are:
Pipeline span buffer coherency
Per-material dither type settings, can't think of a use
Texture LUT behavior
the "LoD (does nothing)" checkbox, obviously
large texture mode, if I need large textures I'll handle it manually (for several reasons)

[4]
Particle group objects are still a bit problematic in regards to position, size and rotation, which causes a lot of time spent trying to get them right in game by trial and error.
Also I can handle the blender geometry nodes setup myself, but I can't seem to figure out exactly how the size of the particle instance object (the single one that gets copied all over for each particle) relates to the size in game, it doesn't seem 1:1. Ideally I'd like if the instance ofject is 1 by 1 meter for example, it ends up the same size in game. Actually a max size of say 2 by 2 meters would be cool so we could maybe use it for tree leaves or things like that.
Distance culling setting per particle type would be very useful. Maybe it can be a property of the particle instance object?

[5]
Scene fog: since we're starting to use many materials across different scenes, we really need this. I'm guessing you were probably gonna do it this way already, but I think we should simply let fog color and range be set by the scene properties, if the "set fog" checkbox in the "sources" tab is unchecked. Nothing else is needed, the actual blender setup should still be manually done per material.

[6]
I'd like for the compiler to apply certain object modifiers. With JR I spent a lot of time manually applying modifiers and copying my work file to an export file.
I'd say we could pick which modifiers to apply based on whether they're enabled for viewing and/or rendering, but I might need certain modifiers visible/rendering but not included in the compile. Modifiers can be manually renamed though, so maybe I can just name them "enabled" and "disabled"? 

[7]
I would very much like to have an "ignore" folder/collection in Blender, that I an put anything into that I need but that doesn't end up in the compile. This is also another thing I spent a lot of time on with JR with keeping stuff in the work file but not the export file etc. Ignored files might be used by certain modifiers, so I hope it's possible to remove them only after the modifiers have already been applied. 

[9]
Materials seem to work differently on the overworld than in the maps. I think we talked about this too. We should eventually have material parity between both, even when it comes to LODs (but I'm guessing you were already working toward this anyway).