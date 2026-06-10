
* Material rendering
* Particle group improvements
* Move action indicator float over target instead of in the UI (A to open doors, pickup, ect)
* Texture convert in other modes
* animation scrolling

Details

[4]
Particle group objects are still a bit problematic in regards to position, size and rotation, which causes a lot of time spent trying to get them right in game by trial and error.
Also I can handle the blender geometry nodes setup myself, but I can't seem to figure out exactly how the size of the particle instance object (the single one that gets copied all over for each particle) relates to the size in game, it doesn't seem 1:1. Ideally I'd like if the instance ofject is 1 by 1 meter for example, it ends up the same size in game. Actually a max size of say 2 by 2 meters would be cool so we could maybe use it for tree leaves or things like that.
Distance culling setting per particle type would be very useful. Maybe it can be a property of the particle instance object?


[9]
Materials seem to work differently on the overworld than in the maps. I think we talked about this too. We should eventually have material parity between both, even when it comes to LODs (but I'm guessing you were already working toward this anyway).