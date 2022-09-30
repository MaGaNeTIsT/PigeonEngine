#pragma once

class CSkyBox
{
public:
	struct SkyBoxInfo
	{
		SkyBoxInfo() { ::ZeroMemory(this, sizeof(*this)); }
		SkyBoxInfo(const FLOAT& radius)
		{
			Radius = radius;
		}
		FLOAT Radius;
	};
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	Draw();
protected:
	virtual void	PrepareDraw();
public:
	CSkyBox();
	virtual ~CSkyBox() {}
protected:
	SkyBoxInfo	m_SkyBoxInfo;
	std::shared_ptr<class CPixelShader> m_PixelShader;
protected:
	static std::shared_ptr<class CVertexShader> m_VertexShader;
};