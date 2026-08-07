
* Particle group improvements
* Texture convert in other modes
* animation scrolling
* Embedded materials

Details

[4]
Particle group objects are still a bit problematic in regards to position, size and rotation, which causes a lot of time spent trying to get them right in game by trial and error.
Also I can handle the blender geometry nodes setup myself, but I can't seem to figure out exactly how the size of the particle instance object (the single one that gets copied all over for each particle) relates to the size in game, it doesn't seem 1:1. Ideally I'd like if the instance ofject is 1 by 1 meter for example, it ends up the same size in game. Actually a max size of say 2 by 2 meters would be cool so we could maybe use it for tree leaves or things like that.
Distance culling setting per particle type would be very useful. Maybe it can be a property of the particle instance object?


[9]
Materials seem to work differently on the overworld than in the maps. I think we talked about this too. We should eventually have material parity between both, even when it comes to LODs (but I'm guessing you were already working toward this anyway).

4: Material features sometimes don't seem to work right if I just add a unique non-linked material to an object or scene, for example scrolling doesn't seem to work, sometimes the material just doesn't show up, etc.
Moving the material to a .mat.blend file and linking it always seems to fix any issues. Which isn't normally a problem, but it would be really nice to be able to just add non-linked materials to an object if we know the materials will never be reused for anything else, for example multi-mat entities like the player. 
But most of all when we get to creating the overworld, I think it would be really good to just be able to have all baked distant terrain materials (which will of course not be reused) reside inside the scene file instead of having to create and link 100-something .mat.blend files. 