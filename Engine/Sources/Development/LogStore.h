#pragma once

#include <Base/DataStructure/ObjectBase.h>

#if !(ENGINE_SHIPPING_MODE)

#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/Memory/Memory.h>
#include <Base/Timer/Timer.h>

namespace PigeonEngine
{
	enum ELogType : UINT8
	{
		ELT_LOG,
		ELT_WARNING,
		ELT_ERROR,

		ELT_COUNT
	};

	/*
	* One log line. The formatted text is built once when the line arrives: the file
	* writer, the width measurement and the output panel all ask for the same string,
	* and rebuilding it per frame was one allocation burst per visible row.
	*/
	struct ELog
	{
		ELog() = delete;
		ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp, const BOOL32 InIsShowInImGui);

		PE_NODISCARD const EString& GetText() const;

		ELogType	Type;
		EString		Log;
		// The bracketed tag at the start of the message, without the brackets. Messages
		// without one belong to "General". The panel filters on this without changing
		// any of the PE_LOG_* call sites.
		EString		Category;
		EngineSystemTime::ESystemTime TimeStamp;
		// FALSE keeps a line in the file and out of the window. The shader compiler prints
		// its per stage progress here - hundreds of lines per compile - and the window lays
		// out every retained line on every frame, so a cold cook would tax every frame that
		// follows it.
		BOOL32		bShowInImGui;

	private:
		EString		Formatted;
	};

	/*
	* Session storage behind EEditorLogManager. Not thread safe on its own: the manager
	* guards every call with its data mutex, and the panel reads through the manager's
	* snapshot API. Kept separate so the store logic (cap, counts, categories, pending
	* file lines) can be exercised by a sanity check without touching the live log.
	*/
	class FLogStore final
	{
	public:
		explicit FLogStore(UINT32 InMaxVisibleLines = 10000u);

		void Add(ELogType InType, const EString& InLog, BOOL32 bInShowInImGui);
		void ClearVisible();

		void SnapshotVisible(TArray<TSharedPtr<ELog>>& OutLogs) const;
		void TakePendingFileLines(TArray<EString>& OutLines);
		// Puts a failed batch back in front of the newer lines, preserving order.
		void RequeuePendingFileLines(const TArray<EString>& InLines);

		PE_NODISCARD UINT32 GetVisibleNum() const;
		PE_NODISCARD UINT32 GetPendingFileLineNum() const;
		PE_NODISCARD UINT32 GetRevision() const;
		PE_NODISCARD UINT32 GetCount(ELogType InType) const;

	private:
		void DropOldestVisible();
		void CompactVisible();

		PE_NODISCARD UINT32 GetLiveVisibleNum() const;

	private:
		TArray<TSharedPtr<ELog>>	VisibleLogs;
		// First live entry of VisibleLogs. Ring eviction only advances this cursor; the
		// buffer is compacted in one move when the dead prefix grows past the live half,
		// so a log flood costs O(1) per dropped line instead of O(n).
		UINT32						VisibleHead = 0u;
		UINT32						MaxVisibleLines = 10000u;
		TArray<EString>				PendingFileLines;
		UINT32						LevelCounts[ELT_COUNT] = { 0u, 0u, 0u };
		UINT32						Revision = 0u;
	};
}

#endif
