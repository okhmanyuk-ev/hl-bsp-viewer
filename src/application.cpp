#include "application.h"

Application::Application() : Shared::Application("hl_bsp_viewer", { Flag::Scene })
{
	PLATFORM->setTitle(PRODUCT_NAME);

	mCamera = std::make_shared<skygfx::utils::PerspectiveCamera>();
	mCameraController = std::make_shared<Shared::FirstPersonCameraController>(mCamera);

	mCamera->yaw = glm::radians(90.0f);
	mCamera->position = { 0.0f, 0.0f, -300.0f };
	mCamera->world_up = { 0.0f, -1.0f, 0.0f };

	mCameraController->setSensivity(1.0f);
	mCameraController->setSpeed(5.0f);

	CONSOLE->execute("hud_show_fps 1");

	//mBSPFile.loadFromFile("de_dust2.bsp", true);
	mBSPFile.loadFromFile("cs_assault.bsp", true);

	auto& textures = mBSPFile.getTextures();
	auto& wads = mBSPFile.getWADFiles();
	std::unordered_map<TexId, std::shared_ptr<skygfx::Texture>> textures_map;

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

				textures_map[tex_id] = std::make_shared<skygfx::Texture>(miptex.width, miptex.height,
					skygfx::PixelFormat::RGBA8UNorm, pixels.data(), true);
			}
		}
	}

	auto model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

	mBspDraw = std::make_shared<HL::BspDraw>(mBSPFile, textures_map, model_matrix);

	GRAPHICS->setBatching(false); // bug in batching, 3D objects cannot be rendered

	//mCamera->setPosition({ 359.867f, -368.278f, 1721.268f });
	mCamera->position = { 1441.438f, -61.265f, 802.192f };

	auto lights = mBspDraw->getLights();

	lights.push_back(skygfx::utils::DirectionalLight{
		.direction = { 0.75f, 0.75f, 0.75f },
		//.ambient = Graphics::Color::ToNormalized(23, 22, 22),
		//.diffuse = Graphics::Color::ToNormalized(65, 65, 65),
		//.specular = Graphics::Color::ToNormalized(54, 51, 51),
		.ambient = Graphics::Color::ToNormalized(116, 103, 103),
		.diffuse = Graphics::Color::ToNormalized(139, 176, 147),
		.specular = Graphics::Color::ToNormalized(255, 255, 255),
	});

	lights.push_back(skygfx::utils::PointLight{
		.position = { 151.977f, -326.671f, 1629.473f },
		.ambient = { 1.0f, 1.0f, 1.0f },
		.diffuse = { 0.25f, 1.0f, 0.25f },
		.specular = { 0.25f, 0.25f, 1.0f },
		.constant_attenuation = 0.0f,
		.linear_attenuation = 0.012f,
		.quadratic_attenuation = 0.0f
	});

	mBspDraw->setLights(lights);

	//mCamera->setPitch(glm::radians(12.0f));
	//mCamera->setYaw(glm::radians(-138.0f));
	mCamera->pitch = glm::radians(-13.0f);
	mCamera->yaw = glm::radians(155.0f);

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
	mBloomLayer->setBrightThreshold(2.0f);
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
	auto directionalLight = std::get<skygfx::utils::DirectionalLight>(mBspDraw->getLights().at(0));
	auto pointLight = std::get<skygfx::utils::PointLight>(mBspDraw->getLights().at(1));

	static bool show_settings = false;

	ImGui::Begin("Settings", nullptr, ImGui::User::ImGuiWindowFlags_Overlay & ~ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImGui::User::BottomLeftCorner());

	if (show_settings)
	{
		ImGui::SliderAngle("Pitch##1", &mCamera->pitch, -89.0f, 89.0f);
		ImGui::SliderAngle("Yaw##1", &mCamera->yaw, -180.0f, 180.0f);
		ImGui::SliderFloat("Fov##1", &mCamera->fov, 1.0f, 180.0f);
		ImGui::DragFloat3("Position##1", (float*)&mCamera->position);

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
			pointLight.position = mCamera->position;
		ImGui::DragFloat("Constant Attenuation##3", &pointLight.constant_attenuation, 0.01f);
		ImGui::DragFloat("Linear Attenuation##3", &pointLight.linear_attenuation, 0.001f);
		ImGui::DragFloat("Quadratic Attenuation##3", &pointLight.quadratic_attenuation, 0.0001f);
		ImGui::ColorEdit3("Ambient##3", (float*)&pointLight.ambient);
		ImGui::ColorEdit3("Diffuse##3", (float*)&pointLight.diffuse);
		ImGui::ColorEdit3("Specular##3", (float*)&pointLight.specular);

		ImGui::Separator();
		ImGui::Checkbox("Light Animation", &mLightAnimation);

		ImGui::Separator();

		auto bloom_enabled = mBloomLayer->isPostprocessEnabled();
		auto threshold = mBloomLayer->getBrightThreshold();
		auto glow = mBloomLayer->getIntensity();

		ImGui::Checkbox("Bloom Enabled", &bloom_enabled);
		ImGui::SliderFloat("Bright Threshold", &threshold, 0.0f, 4.0f);
		ImGui::SliderFloat("Glow Intensity", &glow, 0.0f, 8.0f);

		mBloomLayer->setPostprocessEnabled(bloom_enabled);
		mBloomLayer->setBrightThreshold(threshold);
		mBloomLayer->setIntensity(glow);
	}

	ImGui::Checkbox("Settings", &show_settings);

	ImGui::End();

	if (mLightAnimation)
		pointLight.position = mLightPosition;

	auto lights = mBspDraw->getLights();
	lights[0] = directionalLight;
	lights[1] = pointLight;
	mBspDraw->setLights(lights);

	mBspDraw->draw(mSceneTarget, mCamera->position, mCamera->yaw, mCamera->pitch, mCamera->fov, mCamera->world_up);

	//

	glm::vec3 pos = {
		mCamera->position.x,
		mCamera->position.z,
		-mCamera->position.y,
	};

	auto vectors = skygfx::utils::MakePerspectiveCameraVectors(*mCamera);

	glm::vec3 dir = {
		vectors.front.x,
		vectors.front.z,
		-vectors.front.y,
	};

	auto trace = mBSPFile.traceLine(pos, pos + (dir * 8192.0f));

	skygfx::utils::Mesh::Vertex v1, v2, v3, v4, v5, v6, v7;

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

	skygfx::utils::Mesh::Vertices vertices = {
		v1, v2,
		v1, v3,
		v1, v4,
		v1, v5,
		v1, v6,
		v1, v7
	};

	auto matrices = skygfx::utils::MakeCameraMatrices(*mCamera);
	auto model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

	GRAPHICS->begin();
	GRAPHICS->pushRenderTarget(mSceneTarget);
	GRAPHICS->pushProjectionMatrix(matrices.projection);
	GRAPHICS->pushViewMatrix(matrices.view);
	GRAPHICS->pushModelMatrix(model);
	GRAPHICS->pushDepthMode(skygfx::ComparisonFunc::Less);
	GRAPHICS->draw(nullptr, nullptr, skygfx::Topology::LineList, vertices, {});
	GRAPHICS->pop(5);
	GRAPHICS->end();
}

void Application::onEvent(const Platform::System::ResizeEvent& e)
{
	if (e.width == 0 && e.height == 0)
		return;

	mSceneTarget = std::make_shared<skygfx::RenderTarget>(e.width, e.height);
	mSceneSprite->setTexture(mSceneTarget);
}
