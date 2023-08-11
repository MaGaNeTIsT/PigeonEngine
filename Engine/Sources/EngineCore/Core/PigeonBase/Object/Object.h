#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class PObject : public EObjectBase
	{

		//EClass(PObject, EObjectBase)

	public:
		virtual void	Init() = 0;
		virtual void	Uninit();
		virtual void	Tick(FLOAT deltaTime) = 0;
#ifdef _DEVELOPMENT_EDITOR
		virtual void	EditorTick(FLOAT deltaTime) = 0;
	public:
		void	SetName(const std::string& name);
#endif
	public:
		virtual BOOL	IsActive()const;
		virtual void	Active();
		virtual void	Inactive();
	protected:
		BOOL			m_Active;

		CLASS_VIRTUAL_NOCOPY_BODY(PObject)


	public:
		void SetIsTickable(const BOOL& bTickable);
		BOOL IsTickable() const;
	private:
		BOOL bCanTick = false;
	public:
		virtual void Destroy();
	};

};