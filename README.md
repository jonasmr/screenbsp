#Screenbsp

Screenbsp is _failed_ attempt at creating a new visibility system, intended to replace software rasterization and/or cell/portal systems which are still used in games.
Its based on an idea suggested to me by Simon Mogensen: I believe it might've been used in some very early games.

#Disclaimer
Note: I consider this a failure, for various reasons. I wouldn't recommend using this approach. I'm releasing this in case others find it interesting. If nothing else, I think its a novel idea.
The code is as-is. Somewhat optimized, but not at all fully optimized. And there are cases where it does not work. 
Most of it is probably a mess.


#Construction
Given a set of occluders, Quads, a screenspace bsp tree is constructed every frame. For each Quad, Five planes are added, one for each quad of the box. For each Quad, For each edge[e0-e1] a plane is constructed such that the lies within the plane and the plane normal is identical to normalize(cross(e0-e11,eye-p0). These four planes, with the plane of the quad itself now defines a space which can be considered as cullable - IE if an object is inside all five it can be culled.

#Culling

To cull an object, a camera facing quad, fully covering the OBB in screenspace of the object is constructed. This quad is then clipped in the bsp tree.

The advantages to this method is:
* It fully supports occluder fusion. Since clipping is done with the BSP tree, we can be partially occluded from multiple occluders, and still be occluded. The only other culling algorithm I'm aware of that supports occluder fusion is software rasterization.
* It is exact. Software rasterization is not.


# Validation
The demo program contains a verification pass, which renders the entire scene with occlusion queries, to validate the result. It _almost_ works - but if you run the sample verification with enough passes it will eventually find some cases where it fails. 

# Problems
Theoretically this approach is really awesome. In practice not so much.

* Numerical stability is a pain. I believe I got it relatively stable, mostly by just fucking around. Getting this right was probably the main reason I gave up on this idea.
* Multithreaded creation of the bsp tree is not possible. Testing should be fine.
* Cull test time is dependent on the tree constructed. It can be a bit unreliable
* Debugging BSP trees is _painful_. Most of the time spent with this code was spent trying to figure out why it didnt work.




# License
Licensed using unlicense.org