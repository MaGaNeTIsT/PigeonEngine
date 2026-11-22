#include <Development/LogStore.h>

#if !(ENGINE_SHIPPING_MODE)

#include <cstdio>

namespace PigeonEngine
{
	namespace
	{
		EString FormatPadded(const UINT32 InValue, const UINT32 InWidth)
		{
			CHAR Buffer[16] = { 0 };
			::snprintf(Buffer, sizeof(Buffer), "%0*u", InWidth, InValue);
			return EString(Buffer);
		}

		EString FormatTimestamp(const EngineSystemTime::ESystemTime& InTime)
		{
			EString Result;
			Result += FormatPadded(InTime.Year, 4u);
			Result += "-";
			Result += FormatPadded(InTime.Month, 2u);
			Result += "-";
			Result += FormatPadded(InTime.Day, 2u);
			Result += " ";
			Result += FormatPadded(InTime.Hour, 2u);
			Result += ":";
			Result += FormatPadded(InTime.Minute, 2u);
			Result += ":";
			Result += FormatPadded(InTime.Second, 2u);
			Result += ".";
			Result += FormatPadded(InTime.Milliseconds, 3u);
			return Result;
		}

		EString ParseCategory(const EString& InLog)
		{
			// Most engine sites tag their messages with a leading [System] instead of
			// passing a category. Parsing it here gives the panel a filter dimension
			// without touching the PE_LOG_* macros or their hundreds of call sites.
			if ((InLog.Length() < 3u) || (InLog[0] != '['))
			{
				return EString("General");
			}
			const INT32 CloseIndex = InLog.Find("]");
			if (CloseIndex <= 1)
			{
				return EString("General");
			}
			return InLog.Substring(1u, static_cast<UINT32>(CloseIndex - 1));
		}

		const CHAR* GetLevelLabel(const ELogType InType)
		{
			switch (InType)
			{
			case ELT_WARNING:
				return "Warning: ";
			case ELT_ERROR:
				return "Error: ";
			default:
				return "Log: ";
			}
		}
	}

	ELog::ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp
		, const BOOL32 InIsShowInImGui)
		:
		Type(InType),
		Log(InLog),
		TimeStamp(InTimeStamp),
		bShowInImGui(InIsShowInImGui)
	{
		Category = ParseCategory(InLog);
		// One allocation per line, shared by the file writer, the width measurement and
		// the panel. A message containing a percent sign stays data: nothing here feeds
		// the text to a printf-style path.
		Formatted = EString("[") + FormatTimestamp(TimeStamp) + "] " + GetLevelLabel(Type) + Log;
	}

	const EString& ELog::GetText() const
	{
		return Formatted;
	}

	FLogStore::FLogStore(UINT32 InMaxVisibleLines)
	{
		MaxVisibleLines = (InMaxVisibleLines > 0u) ? InMaxVisibleLines : 1u;
	}

	void FLogStore::Add(ELogType InType, const EString& InLog, BOOL32 bInShowInImGui)
	{
		const TSharedPtr<ELog> Entry = EMemory::MakeShared<ELog>(InType, InLog, EngineSystemTime::Now(), bInShowInImGui);
		// Every line belongs in the file, including the file-only progress lines.
		PendingFileLines.Add(Entry->GetText());

		if (bInShowInImGui == FALSE)
		{
			return;
		}

		VisibleLogs.Add(Entry);
		if (static_cast<UINT32>(InType) < static_cast<UINT32>(ELT_COUNT))
		{
			LevelCounts[static_cast<UINT32>(InType)]++;
		}
		while (GetLiveVisibleNum() > MaxVisibleLines)
		{
			DropOldestVisible();
		}
		CompactVisible();
		Revision++;
	}

	void FLogStore::ClearVisible()
	{
		// The file keeps everything. Clear only drops what the window can scroll to.
		VisibleLogs.Empty();
		VisibleHead = 0u;
		for (UINT32 LevelIndex = 0u; LevelIndex < static_cast<UINT32>(ELT_COUNT); LevelIndex++)
		{
			LevelCounts[LevelIndex] = 0u;
		}
		Revision++;
	}

	void FLogStore::SnapshotVisible(TArray<TSharedPtr<ELog>>& OutLogs) const
	{
		OutLogs.Empty();
		const UINT32 VisibleNum = VisibleLogs.Num<UINT32>();
		OutLogs.Reserve(GetLiveVisibleNum());
		for (UINT32 Index = VisibleHead; Index < VisibleNum; Index++)
		{
			OutLogs.Add(VisibleLogs[Index]);
		}
	}

	void FLogStore::TakePendingFileLines(TArray<EString>& OutLines)
	{
		OutLines.Empty();
		OutLines.Reserve(PendingFileLines.Num<UINT32>());
		for (INT32 Index = 0, LineNum = PendingFileLines.Num<INT32>(); Index < LineNum; Index++)
		{
			OutLines.Add(PendingFileLines[Index]);
		}
		PendingFileLines.Empty();
	}

	void FLogStore::RequeuePendingFileLines(const TArray<EString>& InLines)
	{
		// A failed append must not drop the batch. The newer lines arrived after it, so
		// the failed batch goes first and the next flush writes the session in order.
		TArray<EString> Merged;
		Merged.Reserve(InLines.Num<UINT32>() + PendingFileLines.Num<UINT32>());
		for (INT32 Index = 0, LineNum = InLines.Num<INT32>(); Index < LineNum; Index++)
		{
			Merged.Add(InLines[Index]);
		}
		for (INT32 Index = 0, LineNum = PendingFileLines.Num<INT32>(); Index < LineNum; Index++)
		{
			Merged.Add(PendingFileLines[Index]);
		}
		PendingFileLines.Empty();
		PendingFileLines.Append(Merged);
	}

	UINT32 FLogStore::GetVisibleNum() const
	{
		return GetLiveVisibleNum();
	}

	UINT32 FLogStore::GetPendingFileLineNum() const
	{
		return PendingFileLines.Num<UINT32>();
	}

	UINT32 FLogStore::GetRevision() const
	{
		return Revision;
	}

	UINT32 FLogStore::GetCount(ELogType InType) const
	{
		const UINT32 LevelIndex = static_cast<UINT32>(InType);
		return (LevelIndex < static_cast<UINT32>(ELT_COUNT)) ? LevelCounts[LevelIndex] : 0u;
	}

	void FLogStore::DropOldestVisible()
	{
		if (GetLiveVisibleNum() == 0u)
		{
			return;
		}
		const TSharedPtr<ELog>& Oldest = VisibleLogs[VisibleHead];
		if (Oldest)
		{
			const UINT32 LevelIndex = static_cast<UINT32>(Oldest->Type);
			if ((LevelIndex < static_cast<UINT32>(ELT_COUNT)) && (LevelCounts[LevelIndex] > 0u))
			{
				LevelCounts[LevelIndex]--;
			}
		}
		VisibleHead++;
	}

	void FLogStore::CompactVisible()
	{
		if (VisibleHead == 0u)
		{
			return;
		}
		const UINT32 VisibleNum = VisibleLogs.Num<UINT32>();
		// Wait until the dead prefix is both large and at least half the buffer; a
		// compact is a linear move, and doing it per dropped line would put the cost
		// back where the head cursor removed it.
		if ((VisibleHead < MaxVisibleLines) && ((VisibleHead * 2u) < VisibleNum))
		{
			return;
		}
		const UINT32 LiveNum = GetLiveVisibleNum();
		for (UINT32 Index = 0u; Index < LiveNum; Index++)
		{
			VisibleLogs[Index] = EMemory::Move(VisibleLogs[VisibleHead + Index]);
		}
		VisibleLogs.SetNum(LiveNum);
		VisibleHead = 0u;
	}

	UINT32 FLogStore::GetLiveVisibleNum() const
	{
		const UINT32 VisibleNum = VisibleLogs.Num<UINT32>();
		return (VisibleHead < VisibleNum) ? (VisibleNum - VisibleHead) : 0u;
	}
}

#endif
