#pragma once
//PhysicsEngine Include
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>

//Listener
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

//Shapes Include
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>

#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

//Materials Include
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>

//Character Include
#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>