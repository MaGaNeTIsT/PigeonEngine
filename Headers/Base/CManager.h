#pragma once

class CScene;
class CGameTimer;

class CManager
{
private:
	static CScene*				m_Scene;
	static CGameTimer*			m_Timer;
public:
	static void					Init();
	static void					Uninit();
	static void					Update();
	static void					Draw();

	static void					CalculateFrameStats();

	static const CScene*		GetScene() { return m_Scene; }
	static const CGameTimer*	GetTimer() { return m_Timer; }
};