#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	/*
	* Animation evaluation thread contract.
	*
	* EAnimInstance::Update is three stages, and the stage a piece of work belongs to decides
	* whether it can ever move to a worker:
	*
	*   Advance (game thread)
	*     State machine, clip/montage clocks and weights, early-out. This is also the only stage
	*     that may gather game-world inputs: IK targets, ground ray results, gameplay
	*     parameters, anything that needs the component, the actor or the world. Query here and
	*     hand the result to the pose stage as read-only data.
	*
	*   EvaluatePose (worker-safe)
	*     Sampling, crossfade, montage blending, IK solving, bone-to-root matrices. It may read
	*     immutable assets, the binding and the players, and write the passed bone pool plus the
	*     instance's own scratch buffers. It must not touch the component, the actor, the world,
	*     the render queues or a notify callback; a parallel dispatch has no way to serialize
	*     those.
	*
	*   CommitSideEffects (game thread)
	*     Every observable effect: root motion on the actor/component, notify dispatch, montage
	*     section transitions, retiring finished layers, dirty flags.
	*
	* Rules for new evaluation code (IK, layered graphs, procedural bones):
	*   - Put world queries and parameter reads in Advance; put the solve in EvaluatePose.
	*   - Keep per-instance scratch alive across frames; do not add a global mutable cache.
	*   - Treat assets as read-only while evaluating; editor in-place edits (notify refresh,
	*     state machine / montage rebuild) must happen outside evaluation.
	*   - Anything a player or a listener can observe belongs in CommitSideEffects.
	*
	* When to add worker threads (Phase B): only after a representative scene shows
	* GameThread.Anim's per-frame total above roughly 30% of the frame budget while the render
	* thread is not the limiter. Until then, reduce the work or fix the render side first; the
	* split above is what keeps that future change local.
	*/

	/*
	* The state Advance leaves behind for this frame's pose build.
	*
	* Advance is the half of EAnimInstance::Update that moves time: the state machine, the clip
	* players and the montage weights. Everything here is plain data so the pose half
	* (EvaluatePose) can be handed to a worker later without giving it access to the component,
	* the actor or the world.
	*
	* bDefaultPoseOnly is the fallback path: the asset and the skeleton did not bind, so the
	* pose is just the skeleton's default pose. It still needs a pose write when something asked
	* for one (a stop, an asset swap), which is what bShouldEvaluate reports.
	*/
	struct FAnimAdvanceResult
	{
	public:
		BOOL32	bShouldEvaluate			= FALSE;
		BOOL32	bDefaultPoseOnly		= FALSE;
		BOOL32	bAdvancing				= FALSE;
		BOOL32	bPreviousAdvancing		= FALSE;
		BOOL32	bCrossFading			= FALSE;
		BOOL32	bMontageActive			= FALSE;
		BOOL32	bMontageTransitionPending	= FALSE;
		FLOAT	DeltaSeconds			= 0.0f;
	};

	/*
	* The root motion one evaluated frame asks the game thread to move the owner by.
	*
	* EvaluatePose can compute this from the pose alone, but it must not touch the actor: the
	* actor transform is game-thread state and a render snapshot may read it. CommitSideEffects
	* applies it on the game thread, before the frame's notifies run, which is the order the
	* sequential update always used.
	*/
	struct FAnimRootMotionDelta
	{
	public:
		BOOL32		bHasDelta = FALSE;
		Vector3		DeltaPosition = Vector3::Zero();
		Quaternion	DeltaRotation = Quaternion::Identity();
	};

}	// namespace PigeonEngine
