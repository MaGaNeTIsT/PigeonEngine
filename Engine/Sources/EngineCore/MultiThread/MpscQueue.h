#pragma once

#include <CoreMinimal.h>
#include <MultiThread/Atomic.h>
#include <new>

namespace PigeonEngine
{

	/*
	* Multi producer, single consumer lock free queue.
	*
	* This is the engine's one cross thread transport. Every direction the frame needs is an
	* instance of it, with the consumer named by the owner:
	*
	*   any thread            -> render thread   render resource create / destroy commands
	*   any thread            -> load thread     asset load requests
	*
	* Producers never block, never spin and never allocate in a loop: one exchange plus one
	* store publish an element, and the consumer is the only side that walks the chain. That
	* is what makes it safe to hand the queue to a thread that must not stall - a draw, a
	* physics callback, an editor panel.
	*
	* Contract, and it is a hard one:
	*   - Enqueue may be called from any thread, at any time, including while the consumer runs.
	*   - TryDequeue / DequeueAll / Clear may be called from ONE thread only, which is the queue's
	*     consumer. A second consumer would silently steal elements rather than fail, so this is
	*     an interface contract rather than a runtime check.
	*   - IsEmpty / GetPendingCount are consumer calls in the sense that they are exact there;
	*     another thread may call them as a hint.
	*
	* Elements are destroyed by the consumer (or by Clear / the destructor). The element type
	* has to be move constructible; it does not have to be default constructible, copyable or
	* trivially copyable, because the payload lives in raw storage inside the node.
	*/

	template<typename _TElementType>
	class TMpscQueue final
	{
	private:
		/*
		* One linked element. The payload sits in raw storage so the queue accepts a type that
		* cannot be default constructed; only the consumer ever constructs or destroys it.
		*
		* Next is written once: by the producer that linked this node. Every producer that links
		* after it, and the consumer, only read it. A node becomes reachable exactly when its
		* Next store is observed, which is why the consumer may free a node as soon as it has
		* walked past it.
		*/
		struct FNode
		{
		public:
			alignas(_TElementType) BYTE	ElementStorage[sizeof(_TElementType)];
			BOOL32						bHasElement = FALSE;
			TAtomic<FNode*>				Next;
		public:
			PE_FORCEINLINE _TElementType* Element() noexcept
			{
				return (reinterpret_cast<_TElementType*>(ElementStorage));
			}
			PE_FORCEINLINE void Construct(_TElementType&& InElement)
			{
				::new (static_cast<void*>(ElementStorage)) _TElementType(static_cast<_TElementType&&>(InElement));
				bHasElement = TRUE;
			}
			PE_FORCEINLINE void Destroy()
			{
				if (bHasElement)
				{
					Element()->~_TElementType();
					bHasElement = FALSE;
				}
			}
		};

	public:
		TMpscQueue() noexcept
			: Head(nullptr)
			, Tail(nullptr)
			, PendingCount(0u)
		{
			// The tail is a stub: every producer links behind it, so there is always a node to
			// write Next into and the empty state needs no special case anywhere.
			FNode* Stub = new FNode();
			Stub->Next.Store(nullptr, std::memory_order_relaxed);
			Head.Store(Stub, std::memory_order_relaxed);
			Tail = Stub;
		}
		~TMpscQueue()
		{
			Clear();
			delete Tail;
			Tail = nullptr;
			Head.Store(nullptr, std::memory_order_relaxed);
		}

	public:
		/*
		* Any thread. Publishes InElement for the consumer. The element is moved in, so a caller
		* passes what it just built:
		*
		*   Queue.Enqueue(FAssetLoadRequest{ ... });
		*/
		void Enqueue(_TElementType InElement)
		{
			FNode* NewNode = new FNode();
			NewNode->Construct(static_cast<_TElementType&&>(InElement));
			NewNode->Next.Store(nullptr, std::memory_order_relaxed);
			// Two steps and no retry. The exchange claims this node's place in the chain and the
			// store links it behind the node that held the place before it. Only the store makes
			// it reachable, and the store is a release, so a consumer that sees it also sees the
			// fully constructed element.
			FNode* Previous = Head.Exchange(NewNode, std::memory_order_acq_rel);
			Previous->Next.Store(NewNode, std::memory_order_release);
			PendingCount.FetchAdd(1u, std::memory_order_relaxed);
		}

		/*
		* Consumer only. Takes the oldest element, or returns FALSE when the queue is empty.
		* DequeueAll is the usual way to drain a frame; this exists for callers that want one
		* element at a time (a shutdown drain, a bounded work budget).
		*/
		PE_NODISCARD BOOL32 TryDequeue(_TElementType& OutElement)
		{
			FNode* OldTail = Tail;
			FNode* NextNode = OldTail->Next.Load(std::memory_order_acquire);
			if (!NextNode)
			{
				return FALSE;
			}
			// The node that held the element becomes the stub, so the queue keeps exactly one
			// node without an element and the old one is freed here. A node later in the chain
			// may already be linked to it; that is the next call's element.
			Tail = NextNode;
			OutElement = static_cast<_TElementType&&>(*(NextNode->Element()));
			NextNode->Destroy();
			delete OldTail;
			PendingCount.FetchSub(1u, std::memory_order_relaxed);
			return TRUE;
		}

		/*
		* Consumer only. Moves every element that is linked right now into InConsumeFunc, in the
		* order it was published, and destroys it after the call returns. Returns how many were
		* consumed.
		*
		* The batch is detached before it is walked, so a command that enqueues while it runs is
		* not part of this batch: it lands behind the new tail and belongs to the next drain.
		* That is what keeps a re-entrant enqueue from invalidating the walk.
		*/
		template<typename _TConsumeFuncType>
		UINT32 DequeueAll(_TConsumeFuncType&& InConsumeFunc)
		{
			FNode* First = nullptr;
			FNode* Last = nullptr;
			if (!DetachChain(First, Last))
			{
				return 0u;
			}
			UINT32 ConsumedNum = 0u;
			FNode* Current = First;
			while (Current != Last)
			{
				InConsumeFunc(static_cast<_TElementType&&>(*(Current->Element())));
				Current->Destroy();
				FNode* NextNode = Current->Next.Load(std::memory_order_relaxed);
				delete Current;
				Current = NextNode;
				ConsumedNum++;
			}
			// The last node stays behind as the new stub; only its element is consumed.
			InConsumeFunc(static_cast<_TElementType&&>(*(Last->Element())));
			Last->Destroy();
			ConsumedNum++;
			PendingCount.FetchSub(ConsumedNum, std::memory_order_relaxed);
			return ConsumedNum;
		}

		/*
		* Consumer only. Destroys everything that is queued right now without running it. The
		* shutdown form: a request that will never be served is dropped rather than executed
		* against a device or a manager that is already going away.
		*/
		void Clear()
		{
			FNode* First = nullptr;
			FNode* Last = nullptr;
			if (!DetachChain(First, Last))
			{
				return;
			}
			UINT32 ClearedNum = 0u;
			FNode* Current = First;
			while (Current != Last)
			{
				Current->Destroy();
				FNode* NextNode = Current->Next.Load(std::memory_order_relaxed);
				delete Current;
				Current = NextNode;
				ClearedNum++;
			}
			Last->Destroy();
			ClearedNum++;
			PendingCount.FetchSub(ClearedNum, std::memory_order_relaxed);
		}

	public:
		// Consumer exact, other threads a hint.
		PE_NODISCARD BOOL32 IsEmpty() const
		{
			return ((Tail->Next.Load(std::memory_order_acquire) == nullptr) ? TRUE : FALSE);
		}
		// Diagnostics: how many elements are queued right now, as last seen by this thread.
		PE_NODISCARD UINT64 GetPendingCount() const
		{
			return PendingCount.Load(std::memory_order_relaxed);
		}

	private:
		/*
		* Consumer only. Takes the chain of nodes linked right now off the queue and reports its
		* ends. FALSE when nothing is linked.
		*
		* The walk stops at the last node that is linked *yet*: a producer that has already
		* claimed its place in the chain (it is the head now) but has not linked behind this
		* node is simply not part of this batch. It links behind the new stub afterwards and the
		* next drain picks it up.
		*/
		PE_NODISCARD BOOL32 DetachChain(FNode*& OutFirst, FNode*& OutLast)
		{
			FNode* OldTail = Tail;
			FNode* First = OldTail->Next.Load(std::memory_order_acquire);
			if (!First)
			{
				return FALSE;
			}
			FNode* Last = First;
			for (;;)
			{
				FNode* NextNode = Last->Next.Load(std::memory_order_acquire);
				if (!NextNode)
				{
					break;
				}
				Last = NextNode;
			}
			Tail = Last;
			delete OldTail;
			OutFirst = First;
			OutLast = Last;
			return TRUE;
		}

	public:
		TMpscQueue(const TMpscQueue&) = delete;
		TMpscQueue(TMpscQueue&&) = delete;
		TMpscQueue& operator=(const TMpscQueue&) = delete;
		TMpscQueue& operator=(TMpscQueue&&) = delete;

	private:
		// Producers write, the consumer reads. Points at the most recently published node.
		TAtomic<FNode*>	Head;
		// Consumer private. Always points at a node whose element has been consumed, so the
		// chain behind it is exactly what is still waiting.
		FNode*			Tail;
		// Diagnostics only. Exact on the consumer, approximate elsewhere.
		TAtomic<UINT64>	PendingCount;

	};

}	// namespace PigeonEngine
