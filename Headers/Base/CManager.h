#pragma once

class CScene;
class CTimer;

class CManager
{
private:
	static CScene*				m_Scene;
	static CTimer				m_Timer;
public:
	static void					Init();
	static void					Uninit();
	static void					Update();
	static void					Draw();

	static void					CalculateFrameStats();

	static CScene*				GetScene() { return m_Scene; }
};