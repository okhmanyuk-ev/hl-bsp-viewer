#pragma once

#include <HL/bspfile.h>
#include <HL/bsp_draw.h>
#include <shared/all.h>

class Application : public Shared::Application, 
	public Common::FrameSystem::Frameable,
	public Common::Event::Listenable<Platform::System::ResizeEvent>
{
public:
	using TexId = int;

	struct Face
	{
		int start;
		int count;
		TexId tex_id;
	};

public:
	Application();

private:
	void onFrame() override;
	void onEvent(const Platform::System::ResizeEvent& e) override;

private:
	BSPFile mBSPFile;
	std::shared_ptr<HL::BspDraw> mBspDraw;
	std::shared_ptr<skygfx::utils::PerspectiveCamera> mCamera;
	std::shared_ptr<Shared::FirstPersonCameraController> mCameraController;

	bool mLightAnimation = false;
	glm::vec3 mLightPosition;

	std::shared_ptr<skygfx::RenderTarget> mSceneTarget = std::make_shared<skygfx::RenderTarget>(PLATFORM->getWidth(), PLATFORM->getHeight());

	std::shared_ptr<Scene::BloomLayer> mBloomLayer;
	std::shared_ptr<Scene::Sprite> mSceneSprite;
};
