#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <Base/DataStructure/Container/Map.h>

namespace PigeonEngine
{

	/*
	* Render side asset proxies.
	*
	* The two tier model splits every asset in two: the game side asset owns the cold data
	* and the values, and a render side proxy owns the GPU objects built from them. This
	* file is the root of the second half.
	*
	* RAssetProxy adds to RRenderResourceInterface the two things a render side manager
	* needs and a bare interface cannot give it:
	*
	*   - identity. The proxy carries the same name the game side cache keys on, so a proxy
	*     can be found from an asset name alone and a cache can report what it holds.
	*   - a type tag. A lookup returns RAssetProxy*, and the tag is what makes the cast to
	*     the concrete proxy class checkable instead of a matter of trusting the caller.
	*
	* Not every render resource is an asset proxy. The sub resources of an asset (a mesh's
	* vertex / index buffers, a texture's views) stay on RRenderResourceInterface and are
	* reached through their asset's proxy; only the one object that represents the asset on
	* the render thread derives from here.
	*/

	enum ERenderProxyType : UINT8
	{
		RENDER_PROXY_TYPE_UNKNOWN		= 0u,
		RENDER_PROXY_TYPE_SHADER_STAGE,
		RENDER_PROXY_TYPE_SHADER_PARAMETERS,
		RENDER_PROXY_TYPE_MESH,
		RENDER_PROXY_TYPE_SKELETON,
		RENDER_PROXY_TYPE_TEXTURE2D,
		RENDER_PROXY_TYPE_TEXTURE_CUBE,
		RENDER_PROXY_TYPE_COUNT,
	};

	class RAssetProxy : public RRenderResourceInterface
	{
	public:
		RAssetProxy() noexcept
			: ProxyName()
			, ProxyType(ERenderProxyType::RENDER_PROXY_TYPE_UNKNOWN)
		{
		}
		RAssetProxy(const EString& InProxyName, const ERenderProxyType InProxyType)
			: ProxyName(InProxyName)
			, ProxyType(InProxyType)
		{
		}
		virtual ~RAssetProxy() noexcept
		{
		}
	public:
		PE_NODISCARD const EString&		GetProxyName() const noexcept { return ProxyName; }
		PE_NODISCARD ERenderProxyType	GetProxyType() const noexcept { return ProxyType; }
		// TRUE when this proxy was built for exactly this identity, which is the condition
		// under which a lookup result may be cast to the concrete proxy class.
		PE_NODISCARD BOOL32				IsProxyOf(const EString& InProxyName, const ERenderProxyType InProxyType) const noexcept
		{
			return (((ProxyType == InProxyType) && (ProxyName == InProxyName)) ? TRUE : FALSE);
		}
	protected:
		// Called by the concrete proxy once it knows what it was built for.
		void	SetProxyIdentity(const EString& InProxyName, const ERenderProxyType InProxyType)
		{
			ProxyName = InProxyName;
			ProxyType = InProxyType;
		}
	private:
		EString			ProxyName;
		ERenderProxyType	ProxyType;
	};

	/*
	* Render side mirror of one game side asset cache.
	*
	* The game side cache keys on the asset identity and owns the cold data; this one keys on
	* the same identity and owns the proxies built from it. Keeping the two keyed the same way
	* is what lets a render side manager answer "what is loaded, and what am I holding".
	*
	* Render thread only. Entries are created inside the queued creation command and dropped
	* inside the queued destroy command, both of which run on the render thread, so the table
	* needs no lock and never sees a half built proxy.
	*/
	template<typename TKeyType, typename TProxyType>
	class TRenderAssetProxyManager
	{
	public:
		TRenderAssetProxyManager()
		{
		}
		virtual ~TRenderAssetProxyManager()
		{
			ReleaseAll();
		}
	public:
		// Takes ownership of InProxy. FALSE when the key is already taken, which means a
		// creation command ran twice for one asset and the second proxy has to be released
		// by its caller.
		BOOL32	Add(const TKeyType& InKeyValue, TProxyType* InProxy)
		{
			if (!InProxy)
			{
				return FALSE;
			}
			if (Proxies.ContainsKey(InKeyValue))
			{
				return FALSE;
			}
			Proxies.Add(InKeyValue, InProxy);
			return TRUE;
		}
		PE_NODISCARD TProxyType*	Find(const TKeyType& InKeyValue)
		{
			TProxyType* Result = nullptr;
			Proxies.FindValue(InKeyValue, Result);
			return Result;
		}
		PE_NODISCARD const TProxyType*	Find(const TKeyType& InKeyValue) const
		{
			const TProxyType* Result = nullptr;
			Proxies.FindValue(InKeyValue, Result);
			return Result;
		}
		PE_NODISCARD BOOL32	Contain(const TKeyType& InKeyValue) const
		{
			return (Proxies.ContainsKey(InKeyValue));
		}
		// Hands the proxy back to the caller, which owns it again. The destroy path has to
		// release it through the device on its own terms, so this never releases for you.
		TProxyType*	Remove(const TKeyType& InKeyValue)
		{
			TProxyType* Result = nullptr;
			if (Proxies.FindValue(InKeyValue, Result))
			{
				Proxies.Remove(InKeyValue);
			}
			return Result;
		}
		// Releases and deletes every proxy it holds. Only for the teardown path, where the
		// render thread is already stopped and nothing can still be referring to them.
		void	ReleaseAll()
		{
			for (auto It = Proxies.Begin(); It != Proxies.End(); It++)
			{
				TProxyType* Proxy = It->second;
				if (Proxy)
				{
					Proxy->ReleaseRenderResource();
					delete Proxy;
				}
			}
			Proxies.Empty();
		}
		PE_NODISCARD UINT32	Size() const
		{
			return (Proxies.Num<UINT32>());
		}
	protected:
		TMap<TKeyType, TProxyType*>	Proxies;

		CLASS_REMOVE_COPY_BODY(TRenderAssetProxyManager)

	};

} // namespace PigeonEngine
