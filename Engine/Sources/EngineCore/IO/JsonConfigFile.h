#pragma once

#include <CoreMinimal.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{
	/*
	* Structured JSON settings with a tracked default and a runtime sparse override.
	*
	* Paths follow the same working-directory convention as the ImGui layout:
	*   ./Engine/Configs/<Name>.default.json  tracked template (source tree or build copy)
	*   ./Engine/Configs/<Name>.user.json     local, gitignored, stores values that differ
	*
	* Visual Studio runs with the build directory as the working directory, so the default is
	* the copy CMake places in Build/Engine/Configs and the user file stays in the ignored
	* Build tree. Running from the repository root uses the tracked default and a source-tree
	* user file, which the *.user.json ignore rule keeps out of git.
	*
	* Defaults and the effective document are plain CJsonObject DOMs; the JsonConfigFile is
	* only the settings policy on top (schema keys, default -> override layering, sparse save).
	* A key is either "Key" or "Group.Key"; Add* declares its default in the document.
	*
	* Load() starts from the declared defaults, copies the keys present in the default file,
	* then copies the user overrides. Set() updates the effective document and rewrites the
	* user file with the keys that differ from the default file; restoring every value to its
	* default deletes the user file, so the tracked default applies again.
	*/
	class EJsonConfigFile final
	{
	public:
		explicit EJsonConfigFile(const EString& InName);

		// Schema declaration. Calling again with the same key keeps the existing default.
		void AddBool(const EString& InKey, const BOOL32& bInDefault);
		void AddInt(const EString& InKey, const INT32& InDefault);
		void AddFloat(const EString& InKey, const FLOAT& InDefault);
		void AddString(const EString& InKey, const EString& InDefault);

		// Reads the default file, then the user overrides. Missing or invalid files are
		// skipped; a missing default leaves the declared values.
		void Load();
		// Writes only the keys that differ from the default file, deleting the user file when
		// nothing differs.
		void Save() const;

		PE_NODISCARD BOOL32		GetBool(const EString& InKey);
		PE_NODISCARD INT32		GetInt(const EString& InKey);
		PE_NODISCARD FLOAT		GetFloat(const EString& InKey);
		PE_NODISCARD EString	GetString(const EString& InKey);

		// Immediate save: settings are toggled by hand, not per frame, so one write per
		// change keeps the file consistent even if the editor crashes.
		void SetBool(const EString& InKey, const BOOL32& bInValue);
		void SetInt(const EString& InKey, const INT32& InValue);
		void SetFloat(const EString& InKey, const FLOAT& InValue);
		void SetString(const EString& InKey, const EString& InValue);

	private:
		void ApplyFile(const EString& InPath, CJsonObject& OutDocument);

		EString					Name;
		EString					DefaultPath;
		EString					UserPath;
		// Declared values, reset source for Defaults on every Load.
		CJsonObject				Schema;
		// Schema + tracked default file: the baseline saved overrides are compared against.
		CJsonObject				Defaults;
		// Defaults + user overrides: what the getters see.
		CJsonObject				Document;
		TArray<EString>			Keys;
	};
}
