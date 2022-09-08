#  A PROTOTYPE FOR GAME ENGINE WITH D3D11
 Learning engine's part from other game engine or other project. Implement some render pipeline & render features.

#  Class Lists
 Base
  CustomType        : some vector type and defined struct for rendering.
  Timer             : an accurate timer for window and game.
  Manager           : engine management for initialize release updating rendering
  RenderDevice      : graphic API for d3d11 device and context.
  ShaderManager     : load cso files and create d3d11 shader objects. shader can delete by manager or user.
  MeshManager       : create engine mesh type or load mesh file(.obj only for now) for meshrenderer. loaded meshes are deleted automatically when window shut down or by user.
  TextureManager    : load texture file(.tga only for now) and create d3d texture object.
  Shader            : a shader c++ object for user.
  Texture           : a texture c++ object for user.
 Game
  Camera            : scene's camera for rendering.
  Light             : base light for game. directional light only for now.
  Mesh              : a mesh c++ object for user and renderer.
  MeshRenderer      : a renderer for mesh. do a draw call.
  GameObject        : base class for scene's active object with mesh renderer.
 Object
  Cube              : a base game object with cube mesh.
  Plane             : a base game object with plane mesh.
  Polygon2D         : a base game object with screen polygon mesh.

#  Physic system
  Jolt library for 3D physic and physic scene management.

#  Render Pipeline
  Deferred render pipeline.
  GBuffer.
  Lighting.

#  Render Features
  GTAO.
