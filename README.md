#  A PROTOTYPE FOR GAME ENGINE WITH D3D11
  Learning engine's part from other game engine or other project. Implement some render pipeline & render features.<br />
  
##  Class Lists
__Base__<br />
  _CustomType_ : some vector type and defined struct for rendering.<br />
  _Timer_ : an accurate timer for window and game.<br />
  _Manager_ : engine management for initialize release updating rendering.<br />
  _RenderDevice_ : graphic API for d3d11 device and context.<br />
  _ShaderManager_ : load cso files and create d3d11 shader objects. shader can delete by manager or user.<br />
  _MeshManager_ : create engine mesh type or load mesh file(.obj only for now) for meshrenderer. loaded meshes are deleted automatically when window shut down or by user.<br />
  _TextureManager_ : load texture file(.tga only for now) and create d3d texture object.<br />
  _Shader_ : a shader c++ object for user.<br />
  _Texture_ : a texture c++ object for user.<br />
 __Game__<br />
  _Camera_ : scene's camera for rendering.<br />
  _Light_ : base light for game. directional light only for now.<br />
  _Mesh_ : a mesh c++ object for user and renderer.<br />
  _MeshRenderer_ : a renderer for mesh. do a draw call.<br />
  _GameObject_ : base class for scene's active object with mesh renderer.<br />
 __Object__<br />
  _Cube_ : a base game object with cube mesh.<br />
  _Plane_ : a base game object with plane mesh.<br />
  _Polygon2D_ : a base game object with screen polygon mesh.<br />
  
##  Physic system
  Jolt library for 3D physic and physic scene management.<br />
  
##  Render Pipeline
  _Deferred render pipeline_<br />
  _GBuffer_<br />
  _Lighting_<br />
  
##  Render Features
  _GTAO_<br />
  
