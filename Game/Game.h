#pragma once

// The starting point of your game.
// This is a plain IGameObject running on K2EngineLow only.
// Right now it draws nothing, so you will just see the gray clear color.
// Add your own rendering (a triangle, a sprite, a model...) step by step.
class Game : public IGameObject
{
public:
	Game() {}
	~Game() {}
	bool Start();
	void Update();
	void Render(RenderContext& rc);

private:

	struct LightData
	{
		Vector3 ambient;
		float pad0;
		Vector3 direction;
		float pad1;
		Vector3 ligColor;
		float pad2;
		Vector3 eyePos;
		float specPow;
		float specIntensity;
		Vector2 pad3;
	};
	LightData m_light;

	ModelRender m_modelRender;
};
