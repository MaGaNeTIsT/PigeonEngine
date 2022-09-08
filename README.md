#  A PROTOTYPE FOR GAME ENGINE WITH D3D11
 Learning engine's part from other game engine or other project. Implement some render pipeline & render features.\n

#  Class Lists
 Base\n
  CustomType        : some vector type and defined struct for rendering.\n
  Timer             : an accurate timer for window and game.\n
  Manager           : engine management for initialize release updating rendering.\n
  RenderDevice      : graphic API for d3d11 device and context.\n
  ShaderManager     : load cso files and create d3d11 shader objects. shader can delete by manager or user.\n
  MeshManager       : create engine mesh type or load mesh file(.obj only for now) for meshrenderer. loaded meshes are deleted automatically when window shut down or by user.\n
  TextureManager    : load texture file(.tga only for now) and create d3d texture object.\n
  Shader            : a shader c++ object for user.\n
  Texture           : a texture c++ object for user.\n
 Game\n
  Camera            : scene's camera for rendering.\n
  Light             : base light for game. directional light only for now.\n
  Mesh              : a mesh c++ object for user and renderer.\n
  MeshRenderer      : a renderer for mesh. do a draw call.\n
  GameObject        : base class for scene's active object with mesh renderer.\n
 Object\n
  Cube              : a base game object with cube mesh.\n
  Plane             : a base game object with plane mesh.\n
  Polygon2D         : a base game object with screen polygon mesh.\n

#  Physic system
  Jolt library for 3D physic and physic scene management.\n

#  Render Pipeline
  Deferred render pipeline.\n
  GBuffer.\n
  Lighting.\n

#  Render Features
  GTAO.\n
