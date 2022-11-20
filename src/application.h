#pragma once

#include <HL/bspfile.h>
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
	std::shared_ptr<Graphics::Camera3D> mCamera;
	std::shared_ptr<Shared::FirstPersonCameraController> mCameraController;

	using Vertex = skygfx::Vertex::PositionTextureNormal;

	//std::shared_ptr<Renderer::Shaders::Light> mShader = std::make_shared<Renderer::Shaders::Light>(Vertex::Layout, std::set<Renderer::Shaders::Light::Flag>{ });
	std::shared_ptr<Renderer::Shaders::Light> mShader = std::make_shared<Renderer::Shaders::Light>(Vertex::Layout);

	std::vector<Vertex> mVertices;
	std::vector<Face> mFaces;
	std::map<TexId, std::shared_ptr<skygfx::Texture>> mTextures;
	
	bool mLightAnimation = false;
	glm::vec3 mLightPosition;

	std::shared_ptr<skygfx::RenderTarget> mSceneTarget = std::make_shared<skygfx::RenderTarget>(PLATFORM->getWidth(), PLATFORM->getHeight());

	std::shared_ptr<Scene::BloomLayer> mBloomLayer;
	std::shared_ptr<Scene::Sprite> mSceneSprite;
};
