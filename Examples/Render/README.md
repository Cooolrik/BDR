Main development test example. Current focus is on high levels of triangle and object throughput.

* GPU-only based culling:
  * Frustum
  * Rejection cone
  * Depth pyramid of previous frame
* Renders high poly meshes divided into chunks of 512 tris, which are individually culled on GPU
