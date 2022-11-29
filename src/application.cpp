#include "application.h"

Application::Application() : Shared::Application("hl_bsp_viewer", { Flag::Scene })
{
	PLATFORM->setTitle(PRODUCT_NAME);

	mCamera = std::make_shared<Graphics::Camera3D>();
	mCameraController = std::make_shared<Shared::FirstPersonCameraController>(mCamera);

	mCamera->setYaw(glm::radians(90.0f));
	mCamera->setPosition({ 0.0f, 0.0f, -300.0f });

	mCameraController->setSensivity(1.0f);
	mCameraController->setSpeed(5.0f);

	CONSOLE->execute("hud_show_fps 1");

	mBSPFile.loadFromFile("de_dust2.bsp", true);
	//mBSPFile.loadFromFile("cs_assault.bsp", true);

	auto& vertices = mBSPFile.getVertices();
	auto& edges = mBSPFile.getEdges();
	auto& faces = mBSPFile.getFaces();
	auto& surfedges = mBSPFile.getSurfEdges();
	auto& texinfos = mBSPFile.getTexInfos();
	auto& planes = mBSPFile.getPlanes();
	auto& textures = mBSPFile.getTextures();
	auto& wads = mBSPFile.getWADFiles();
	
	//for (auto& texture : textures)
	for (int tex_id = 0; tex_id < textures.size(); tex_id++)
	{
		auto& texture = textures[tex_id];

		for (auto& wad : wads)
		{
			for (auto& lump : wad->getLumps())
			{
				auto& bf = wad->getBuffer();

				bf.setPosition(lump.filepos);

				auto miptex = bf.read<miptex_t>();

				if (std::string(miptex.name) != std::string(texture.name))
					continue;

				auto start = bf.getPosition();

				bf.seek(miptex.height * miptex.width);
				bf.seek((miptex.height / 2) * (miptex.width / 2));
				bf.seek((miptex.height / 4) * (miptex.width / 4));
				bf.seek((miptex.height / 8) * (miptex.width / 8));
				bf.seek(2); // unknown 2 bytes

				auto picture = bf.getPosition();

				struct Pixel
				{
					uint8_t r;
					uint8_t g;
					uint8_t b;
					uint8_t a;
				};

				std::vector<Pixel> pixels;
				pixels.resize(miptex.width * miptex.height);
				
				int j = 0;

				for (auto& pixel : pixels)
				{
					bf.setPosition(start + j);
					bf.setPosition(picture + (bf.read<uint8_t>() * 3));

					j += 1;

					pixel.r = bf.read<uint8_t>();
					pixel.g = bf.read<uint8_t>();
					pixel.b = bf.read<uint8_t>();
					pixel.a = 255;

					if (pixel.r == 0 && pixel.g == 0 && pixel.b == 255)
					{
						pixel.a = 0;
						pixel.r = 255;
						pixel.g = 255;
						pixel.b = 255;
					}
				}

				mTextures[tex_id] = std::make_shared<skygfx::Texture>(miptex.width, miptex.height, 4, pixels.data(), true);
			}
		}
	}

	for (auto& face : faces)
	{
		auto texinfo = texinfos[face.texinfo];
		auto plane = planes[face.planenum];
		auto texture = textures[texinfo._miptex];

		float is = 1.0f / (float)texture.width;
		float it = 1.0f / (float)texture.height;

		Face f = {};

		f.tex_id = texinfo._miptex;
		f.start = (uint32_t)mVertices.size();

		for (int i = face.firstedge; i < face.firstedge + face.numedges; i++)
		{
			auto& surfedge = surfedges[i];
			auto& edge = edges[std::abs(surfedge)];
			auto& pos = vertices[edge.v[surfedge < 0 ? 1 : 0]];

			auto v = Vertex();

			v.pos = pos;
			v.normal = *(glm::vec3*)(&plane.normal);

			if (face.side)
				v.normal = -v.normal;
			
			glm::vec3 ti0 = {
				texinfo.vecs[0][0],
				texinfo.vecs[0][1],
				texinfo.vecs[0][2]
			};

			glm::vec3 ti1 = {
				texinfo.vecs[1][0],
				texinfo.vecs[1][1],
				texinfo.vecs[1][2]
			};

			float s = glm::dot(v.pos, ti0) + texinfo.vecs[0][3];
			float t = glm::dot(v.pos, ti1) + texinfo.vecs[1][3];

			v.texcoord.x = s * is;
			v.texcoord.y = t * it;
		
			if (f.count >= 3) // triangulation 
			{
				f.count += 2;
				mVertices.push_back(mVertices[f.start]);
				mVertices.push_back(mVertices[mVertices.size() - 2]);
			}

			f.count += 1;
			mVertices.push_back(v);
		}

		mFaces.push_back(f);
	}

	GRAPHICS->setBatching(false); // bug in batching, 3D objects cannot be rendered

	//

	//mCamera->setPosition({ 359.867f, -368.278f, 1721.268f });
	mCamera->setPosition({ 1441.438f, -61.265f, 802.192f });

	auto dirLight = mShader->getDirectionalLight();
	dirLight.direction = { 0.75f, 0.75f, 0.75f };
	//dirLight.ambient = Graphics::Color::ToNormalized(23, 22, 22);
	//dirLight.diffuse = Graphics::Color::ToNormalized(65, 65, 65);
	//dirLight.specular = Graphics::Color::ToNormalized(54, 51, 51);
	dirLight.ambient = Graphics::Color::ToNormalized(116, 103, 103);
	dirLight.diffuse = Graphics::Color::ToNormalized(139, 176, 147);
	dirLight.specular = Graphics::Color::ToNormalized(255, 255, 255);

	mShader->setDirectionalLight(dirLight);

	auto pointLight = mShader->getPointLight();
	pointLight.position = { 151.977f, -326.671f, 1629.473f };
	pointLight.constantAttenuation = 0.0f;
	pointLight.linearAttenuation = 0.005f;
	pointLight.quadraticAttenuation = 0.0f;
	pointLight.ambient = { 1.0f, 1.0f, 1.0f };
	pointLight.diffuse = { 0.25f, 1.0f, 0.25f };
	pointLight.specular = { 0.25f, 0.25f, 1.0f };
	mShader->setPointLight(pointLight);

	auto material = mShader->getMaterial();
	material.ambient = { 1.0f, 1.0f, 1.0f };
	material.diffuse = { 1.0f, 1.0f, 1.0f };
	material.specular = { 1.0f, 1.0f, 1.0f };
	mShader->setMaterial(material);

	//mCamera->setPitch(glm::radians(12.0f));
	//mCamera->setYaw(glm::radians(-138.0f));
	mCamera->setPitch(glm::radians(-13.0f));
	mCamera->setYaw(glm::radians(155.0f));

	// light animation

	Actions::Run(Actions::Collection::RepeatInfinite([this] {
		static const std::vector<glm::vec3> Waypoints = {
			{ 420.0f, -340.0f, 1648.0f },
			{ -656.0f, -340.0f, 1648.0f },
			{ -649.0f, -371.0f, 2379.0f },
			{ -549.0f, -379.0f, 2810.0f },
			{ 384.0f, -392.0f, 2820.0f }
		};

		auto seq = Actions::Collection::MakeSequence();

		for (auto waypoint : Waypoints)
		{
			seq->add(Actions::Collection::Interpolate(waypoint, 10.0f, mLightPosition));
		}
		
		return seq;
	}));

	mBloomLayer = std::make_shared<Scene::BloomLayer>();
	mBloomLayer->setStretch(1.0f);
	mBloomLayer->setGlowIntensity(4.0f);
	mBloomLayer->setBlurPasses(8);
	mBloomLayer->setBrightThreshold(0.975f);
	mBloomLayer->setDownscaleFactor(8.0f);
	getScene()->getRoot()->attach(mBloomLayer);

	mSceneSprite = std::make_shared<Scene::Sprite>();
	mSceneSprite->setTexture(mSceneTarget);
	mSceneSprite->setStretch(1.0f);
	mBloomLayer->attach(mSceneSprite);
	
#if defined(PLATFORM_MAC)
	PLATFORM->resize(1600, 1200);
#endif
}

void Application::onFrame()
{
	mCamera->onFrame(); // camera matrices are outdated for 1 frame
	
	auto position = mCamera->getPosition();
	auto pitch = mCamera->getPitch();
	auto yaw = mCamera->getYaw();
	auto directionalLight = mShader->getDirectionalLight();
	auto pointLight = mShader->getPointLight();
	auto material = mShader->getMaterial();

	static bool show_settings = false;
	
	ImGui::Begin("Settings", nullptr, ImGui::User::ImGuiWindowFlags_Overlay & ~ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImGui::User::BottomLeftCorner());

	if (show_settings)
	{
		ImGui::SliderAngle("Pitch##1", &pitch, -89.0f, 89.0f);
		ImGui::SliderAngle("Yaw##1", &yaw, -180.0f, 180.0f);
		ImGui::DragFloat3("Position##1", (float*)&position);

		ImGui::Separator();
		ImGui::Text("Directional Light");
		ImGui::DragFloat3("Direction##2", (float*)&directionalLight.direction, 0.1f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Ambient##2", (float*)&directionalLight.ambient);
		ImGui::ColorEdit3("Diffuse##2", (float*)&directionalLight.diffuse);
		ImGui::ColorEdit3("Specular##2", (float*)&directionalLight.specular);

		ImGui::Separator();
		ImGui::Text("Point Light");
		ImGui::DragFloat3("Position##3", (float*)&pointLight.position);
		ImGui::SameLine();
		if (ImGui::Button("Here"))
			pointLight.position = position;
		ImGui::DragFloat("Constant Attenuation##3", &pointLight.constantAttenuation, 0.01f);
		ImGui::DragFloat("Linear Attenuation##3", &pointLight.linearAttenuation, 0.001f);
		ImGui::DragFloat("Quadratic Attenuation##3", &pointLight.quadraticAttenuation, 0.0001f);
		ImGui::ColorEdit3("Ambient##3", (float*)&pointLight.ambient);
		ImGui::ColorEdit3("Diffuse##3", (float*)&pointLight.diffuse);
		ImGui::ColorEdit3("Specular##3", (float*)&pointLight.specular);

		ImGui::Separator();
		ImGui::Text("Material");
		ImGui::ColorEdit3("Ambient##4", (float*)&material.ambient);
		ImGui::ColorEdit3("Diffuse##4", (float*)&material.diffuse);
		ImGui::ColorEdit3("Specular##4", (float*)&material.specular);
		ImGui::DragFloat("Shininess##4", &material.shininess, 1.0f);

		ImGui::Separator();
		ImGui::Checkbox("Light Animation", &mLightAnimation);

		ImGui::Separator();

		auto bloom_enabled = mBloomLayer->isPostprocessEnabled();
		auto threshold = mBloomLayer->getBrightThreshold();
		auto glow = mBloomLayer->getGlowIntensity();
		auto blur_passes = mBloomLayer->getBlurPasses();
		auto downscale = mBloomLayer->getDownscaleFactor();

		ImGui::Checkbox("Bloom Enabled", &bloom_enabled);
		ImGui::SliderFloat("Bright Threshold", &threshold, 0.0f, 2.0f);
		ImGui::SliderFloat("Glow Intensity", &glow, 0.0f, 8.0f);
		ImGui::SliderInt("Blur Passes", &blur_passes, 1, 8);
		ImGui::SliderFloat("Downscale", &downscale, 1.0f, 8.0f);

		mBloomLayer->setPostprocessEnabled(bloom_enabled);
		mBloomLayer->setBrightThreshold(threshold);
		mBloomLayer->setGlowIntensity(glow);
		mBloomLayer->setBlurPasses(blur_passes);
		mBloomLayer->setDownscaleFactor(downscale);
	}

	ImGui::Checkbox("Settings", &show_settings);

	ImGui::End();

	if (mLightAnimation)
		pointLight.position = mLightPosition;

	mCamera->setPitch(pitch);
	mCamera->setYaw(yaw);
	mCamera->setPosition(position);
	mShader->setEyePosition(position);
	mShader->setDirectionalLight(directionalLight);
	mShader->setPointLight(pointLight);
	mShader->setMaterial(material);

	auto view = mCamera->getViewMatrix();
	auto projection = mCamera->getProjectionMatrix();
	auto model = glm::mat4(1.0f);

	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));

	mShader->setProjectionMatrix(projection);
	mShader->setViewMatrix(view);
	mShader->setModelMatrix(model);
	
	RENDERER->setRenderTarget(mSceneTarget);
	RENDERER->setTopology(skygfx::Topology::TriangleList);
	RENDERER->setDepthMode(skygfx::ComparisonFunc::Less);
	RENDERER->setCullMode(skygfx::CullMode::Back);
	RENDERER->setSampler(skygfx::Sampler::Linear);
	RENDERER->setVertexBuffer(mVertices);
	RENDERER->setShader(mShader);
	RENDERER->setTextureAddressMode(skygfx::TextureAddress::Wrap);
	RENDERER->clear();
	
	for (auto& face : mFaces)
	{
		if (mTextures.count(face.tex_id) == 0)
		{
			RENDERER->draw(face.count, face.start);
			continue;
		}

		RENDERER->setTexture(*mTextures.at(face.tex_id));
		RENDERER->draw(face.count, face.start);
	}

	//

	glm::vec3 pos = {
		mCamera->getPosition().x,
		mCamera->getPosition().z,
		-mCamera->getPosition().y,
	};

	glm::vec3 dir = {
		mCamera->getFront().x,
		mCamera->getFront().z,
		-mCamera->getFront().y,
	};

	auto trace = mBSPFile.traceLine(pos, pos + (dir * 8192.0f));

	skygfx::Vertex::PositionColor v1, v2, v3, v4, v5, v6, v7;

	v1.pos.x = trace.endpos.x;
	v1.pos.y = trace.endpos.y;
	v1.pos.z = trace.endpos.z;

	v2.pos = { v1.pos.x + 10, v1.pos.y,      v1.pos.z };
	v3.pos = { v1.pos.x - 10, v1.pos.y,      v1.pos.z };
	v4.pos = { v1.pos.x,      v1.pos.y + 10, v1.pos.z };
	v5.pos = { v1.pos.x,      v1.pos.y - 10, v1.pos.z };
	v6.pos = { v1.pos.x,      v1.pos.y,      v1.pos.z + 10 };
	v7.pos = { v1.pos.x,      v1.pos.y,      v1.pos.z - 10 };

	v1.color = { Graphics::Color::Lime, 1.0f };
	v2.color = v1.color;
	v3.color = v1.color;
	v4.color = v1.color;
	v5.color = v1.color;
	v6.color = v1.color;
	v7.color = v1.color;

	auto vertices = {
		v1, v2, 
		v1, v3, 
		v1, v4, 
		v1, v5, 
		v1, v6, 
		v1, v7
	};

	GRAPHICS->begin();
	GRAPHICS->pushRenderTarget(mSceneTarget);
	GRAPHICS->pushViewMatrix(mCamera->getViewMatrix());
	GRAPHICS->pushProjectionMatrix(mCamera->getProjectionMatrix());
	GRAPHICS->pushDepthMode(skygfx::ComparisonFunc::Less);
	GRAPHICS->pushModelMatrix(model);
	GRAPHICS->draw(skygfx::Topology::LineList, vertices);
	GRAPHICS->pop(5);
	GRAPHICS->end();

	//getScene()->frame();
}

void Application::onEvent(const Platform::System::ResizeEvent& e)
{
	mSceneTarget = std::make_shared<skygfx::RenderTarget>(e.width, e.height);
	mSceneSprite->setTexture(mSceneTarget);
}
