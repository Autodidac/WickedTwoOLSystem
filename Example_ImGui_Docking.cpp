
#define DPIAWARE
#define INCLUDEICONFONT
#define USEBOUNDSIZING

#include "stdafx.h"
#include "Example_ImGui_Docking.h"

#include "ImGui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "ImGui/imgui_internal.h"

#ifdef _WIN32
#include "ImGui/imgui_impl_win32.h"
#elif defined(SDL2)
#include "ImGui/imgui_impl_sdl.h"
#endif
#include "ImGui/ImGuizmo.h"
#ifdef INCLUDEICONFONT
#include "ImGui/IconsMaterialDesign.h"
#endif

// L System Stuff Here
#include "TwoOLSystem.h"
//#include <WickedRenderer.h>
WickedRenderer treeRenderer = WickedRenderer();
std::vector<LSystemGeneration> generations;
FileManager filemanager;
double simDuration = 40.0; // Simulate for set number of seconds
TimeSimulator timesim;

// Main Menu Stuff
bool show_menu = true; // A flag to control the main menu visibility
bool show_options = false;

ImVec2 image_size(256, 256); // Adjust the size of the image as needed
wi::graphics::Texture my_image_texture;
wi::Resource my_image_resource;
//ImTextureID* my_imgui_texture = nullptr;
void SwitchScene(const std::string& scene_file);
void LoadTexture(const std::string& file_path);

#include "../WickedEngine/wiProfiler.h"
#include "../WickedEngine/wiBacklog.h"
#include "../WickedEngine/wiPrimitive.h"
#include "../WickedEngine/wiRenderPath3D.h"
#include "../Editor/ModelImporter.h"

#include <fstream>
#include <thread>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;
using namespace wi::primitive;

Shader imguiVS;
Shader imguiPS;
Texture fontTexture;
Sampler sampler;
InputLayout	imguiInputLayout;
PipelineState imguiPSO;
ImFont* customfont;
ImFont* customfontlarge;
ImFont* defaultfont;
ImFont* iconfont;
#ifdef _WIN32
HWND hWnd = NULL;
#endif
wi::graphics::SwapChain myswapChain;
void style_dark_ruda(void);
void add_my_font(const char* fontpath);
Example_ImGuiRenderer* active_render = nullptr;

struct ImGui_Impl_Data {};

static ImGui_Impl_Data* ImGui_Impl_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_Impl_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_Impl_CreateDeviceObjects()
{
	auto* backendData = ImGui_Impl_GetBackendData();

	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	TextureDesc textureDesc;
	textureDesc.width = width;
	textureDesc.height = height;
	textureDesc.mip_levels = 1;
	textureDesc.array_size = 1;
	textureDesc.format = Format::R8G8B8A8_UNORM;
	textureDesc.bind_flags = BindFlag::SHADER_RESOURCE;

	SubresourceData textureData;
	textureData.data_ptr = pixels;
	textureData.row_pitch = width * GetFormatStride(textureDesc.format);
	textureData.slice_pitch = textureData.row_pitch * height;

	wi::graphics::GetDevice()->CreateTexture(&textureDesc, &textureData, &fontTexture);

	SamplerDesc samplerDesc;
	samplerDesc.address_u = TextureAddressMode::WRAP;
	samplerDesc.address_v = TextureAddressMode::WRAP;
	samplerDesc.address_w = TextureAddressMode::WRAP;
	samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
	wi::graphics::GetDevice()->CreateSampler(&samplerDesc, &sampler);

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)&fontTexture);

	imguiInputLayout.elements =
	{
		{ "POSITION", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, pos), InputClassification::PER_VERTEX_DATA },
		{ "TEXCOORD", 0, Format::R32G32_FLOAT, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, uv), InputClassification::PER_VERTEX_DATA },
		{ "COLOR", 0, Format::R8G8B8A8_UNORM, 0, (uint32_t)IM_OFFSETOF(ImDrawVert, col), InputClassification::PER_VERTEX_DATA },
	};

	// Create pipeline
	PipelineStateDesc desc;
	desc.vs = &imguiVS;
	desc.ps = &imguiPS;
	desc.il = &imguiInputLayout;
	desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
	desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
	desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);

	desc.pt = PrimitiveTopology::TRIANGLELIST;
	wi::graphics::GetDevice()->CreatePipelineState(&desc, &imguiPSO);

	return true;
}

Example_ImGui::~Example_ImGui()
{
	// Cleanup
	//ImGui_ImplDX11_Shutdown();
#ifdef _WIN32
	ImGui_ImplWin32_Shutdown();
#elif defined(SDL2)
	ImGui_ImplSDL2_Shutdown();
#endif
	ImGui::DestroyContext();
}

void Example_ImGui::Initialize()
{

	// Compile shaders
	{
		auto shaderPath = wi::renderer::GetShaderSourcePath();
		wi::renderer::SetShaderSourcePath(wi::helper::GetCurrentPath() + "/");

		wi::renderer::LoadShader(ShaderStage::VS, imguiVS, "ImGuiVS.cso");
		wi::renderer::LoadShader(ShaderStage::PS, imguiPS, "ImGuiPS.cso");

		wi::renderer::SetShaderSourcePath(shaderPath);
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	#ifdef DPIAWARE
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	#endif
	//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts; //PE: Not really usefull font dont look good.
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	style_dark_ruda();

	add_my_font("Roboto-Medium.ttf");

#ifdef _WIN32
	hWnd = window;
	ImGui_ImplWin32_Init(window);
#elif defined(SDL2)
	ImGui_ImplSDL2_InitForVulkan(window);
#endif

	IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_Impl_Data* bd = IM_NEW(ImGui_Impl_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "Wicked";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	Application::Initialize();

	infoDisplay.active = false;
	infoDisplay.watermark = false;
	infoDisplay.fpsinfo = false;
	infoDisplay.resolution = false;
	infoDisplay.heap_allocation_counter = false;

	renderer.init(canvas);
	renderer.Load();

	active_render = &renderer;

	myswapChain = this->swapChain;

	wi::backlog::Lock();

	ActivatePath(&renderer);
}

void Example_ImGui::Compose(wi::graphics::CommandList cmd)
{
	Application::Compose(cmd);

	// Rendering
	ImGui::Render();

	auto drawData = ImGui::GetDrawData();

	if (!drawData || drawData->TotalVtxCount == 0)
	{
		return;
	}

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	auto* bd = ImGui_Impl_GetBackendData();

	GraphicsDevice* device = wi::graphics::GetDevice();

	// Get memory for vertex and index buffers
	const uint64_t vbSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
	const uint64_t ibSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
	auto vertexBufferAllocation = device->AllocateGPU(vbSize, cmd);
	auto indexBufferAllocation = device->AllocateGPU(ibSize, cmd);

	// Copy and convert all vertices into a single contiguous buffer
	ImDrawVert* vertexCPUMem = reinterpret_cast<ImDrawVert*>(vertexBufferAllocation.data);
	ImDrawIdx* indexCPUMem = reinterpret_cast<ImDrawIdx*>(indexBufferAllocation.data);
	for (int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; cmdListIdx++)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		memcpy(vertexCPUMem, &drawList->VtxBuffer[0], drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(indexCPUMem, &drawList->IdxBuffer[0], drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertexCPUMem += drawList->VtxBuffer.Size;
		indexCPUMem += drawList->IdxBuffer.Size;
	}

	// Setup orthographic projection matrix into our constant buffer
	struct ImGuiConstants
	{
		float   mvp[4][4];
	};

	{
		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

		ImGuiConstants constants;

		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constants.mvp, mvp, sizeof(mvp));

		device->BindDynamicConstantBuffer(constants, 0, cmd);
	}

	const GPUBuffer* vbs[] = {
		&vertexBufferAllocation.buffer,
	};
	const uint32_t strides[] = {
		sizeof(ImDrawVert),
	};
	const uint64_t offsets[] = {
		vertexBufferAllocation.offset,
	};

	device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
	device->BindIndexBuffer(&indexBufferAllocation.buffer, IndexBufferFormat::UINT16, indexBufferAllocation.offset, cmd);

	Viewport viewport;
	viewport.width = (float)fb_width;
	viewport.height = (float)fb_height;
	device->BindViewports(1, &viewport, cmd);

	device->BindPipelineState(&imguiPSO, cmd);

	device->BindSampler(&sampler, 0, cmd);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	int32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	for (uint32_t cmdListIdx = 0; cmdListIdx < (uint32_t)drawData->CmdListsCount; ++cmdListIdx)
	{
		const ImDrawList* drawList = drawData->CmdLists[cmdListIdx];
		for (uint32_t cmdIndex = 0; cmdIndex < (uint32_t)drawList->CmdBuffer.size(); ++cmdIndex)
		{
			const ImDrawCmd* drawCmd = &drawList->CmdBuffer[cmdIndex];
			if (drawCmd->UserCallback)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
				{
				}
				else
				{
					drawCmd->UserCallback(drawList, drawCmd);
				}
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(drawCmd->ClipRect.x - clip_off.x, drawCmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(drawCmd->ClipRect.z - clip_off.x, drawCmd->ClipRect.w - clip_off.y);
				if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				Rect scissor;
				scissor.left = (int32_t)(clip_min.x);
				scissor.top = (int32_t)(clip_min.y);
				scissor.right = (int32_t)(clip_max.x);
				scissor.bottom = (int32_t)(clip_max.y);
				device->BindScissorRects(1, &scissor, cmd);

				const Texture* texture = (const Texture*)drawCmd->TextureId;
				device->BindResource(texture, 0, cmd);
				device->DrawIndexed(drawCmd->ElemCount, indexOffset, vertexOffset, cmd);
			}
			indexOffset += drawCmd->ElemCount;
		}
		vertexOffset += drawList->VtxBuffer.size();
	}
		LoadTexture("../Content/logo_small_copy.png");
/*
	// Check if the menu should be shown
	if (show_menu)
	{

		// Ensure the texture is loaded and valid
		if (my_image_texture.IsValid())
		{
			// Draw the image using Wicked Engine
			//wi::image::Draw(wi::texturehelper::getLogo(), wi::image::Params(75, 40, image_size.x, image_size.y), cmd);
			wi::image::Draw(&my_image_texture, wi::image::Params(75, 40, image_size.x, image_size.y), cmd);
		}
	}*/
}

void Example_ImGuiRenderer::ResizeLayout()
{
	RenderPath3D::ResizeLayout();

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	label.SetPos(XMFLOAT2(screenW / 2.f - label.scale.x / 2.f, screenH * 0.95f));
}

void Example_ImGuiRenderer::Render() const
{
	RenderPath3D::Render();
}

void Example_ImGuiRenderer::Load()
{
	// Reset all state that tests might have modified:
	setFXAAEnabled(true);
	wi::eventhandler::SetVSync(true);
	wi::renderer::ClearWorld(wi::scene::GetScene());
	wi::scene::GetScene().weather = WeatherComponent();
	setAO(RenderPath3D::AO_SSAO);

	this->ClearSprites();
	this->ClearFonts();
	if (wi::lua::GetLuaState() != nullptr)
	{
		wi::lua::KillProcesses();
	}

	// Reset camera position:
	TransformComponent transform;
	transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
	transform.UpdateTransform();
	wi::scene::GetCamera().TransformCamera(transform);

	// Load model.
	wi::scene::LoadModel("../Content/models/bloom_test.wiscene");
			// Create a custom pipe/open ended cylinder
	/*wi::ecs::Entity pipe = wi::scene::GetScene().Entity_CreatePipe("Pipe", 1.0f, 1.0f, 8);
	assert(pipe != wi::ecs::INVALID_ENTITY);
	TransformComponent* transform_base_pipe = wi::scene::GetScene().transforms.GetComponent(pipe);
	transform_base_pipe->Rotate(XMVectorSet(0, 0, 0, 1));
*/
		
	RenderPath3D::Load();
}

#define DIVTWOPI 57.29577951307855
#define IMGUIBUTTONWIDE 200.0f
#define CAMERAMOVESPEED 10.0
bool show_demo_window = false;
bool show_another_window = false;
bool show_performance_data = true;
bool redock_windows_on_start = true;
bool imgui_got_focus = false, imgui_got_focus_last = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
wi::scene::TransformComponent camera_transform;
float camera_pos[3] = { 4.0, 6.0, -15 }, camera_ang[3] = { 5, -15, 0 };
float font_scale = 1.0;
wi::ecs::Entity highlight_entity = -1;
wi::ecs::Entity subset_entity = -1;
ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD); //LOCAL
bool useSnap = false;
float snap[3] = { 0.25f, 0.25f, 0.25f };
float bounds[] = { -1.5f, -1.5f, -1.5f, 1.5f, 1.5f, 1.5f };
float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
bool boundSizing = false;
bool boundSizingSnap = false;
const float identityMatrix[16] =
{   1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f };
bool bIsUsingWidget = false;
int input_text_disable = 0;
int input_text_flags = ImGuiInputTextFlags_ReadOnly;

void Example_ImGuiRenderer::Update(float dt)
{
	// Start the Dear ImGui frame
	auto* backendData = ImGui_Impl_GetBackendData();
	Scene& scene = wi::scene::GetScene();
	CameraComponent& camera = wi::scene::GetCamera();
	ImGuiIO& imgui_io = ImGui::GetIO();
	imgui_got_focus = false;
	bIsUsingWidget = false;

	IM_ASSERT(backendData != NULL);

#ifdef DPIAWARE
	static float dpi_scale = 1.0;
	if (dpi_scale != font_scale)
	{
		font_scale = dpi_scale;
		add_my_font("Roboto-Medium.ttf");
		ImGui_Impl_CreateDeviceObjects();
	}
#endif

	if (!fontTexture.IsValid())
	{
		ImGui_Impl_CreateDeviceObjects();
	}

#ifdef _WIN32
	ImGui_ImplWin32_NewFrame();
#elif defined(SDL2)
	ImGui_ImplSDL2_NewFrame();
#endif
	ImGui::NewFrame();

#ifdef DPIAWARE
	dpi_scale = ImGui::GetWindowDpiScale();
#endif

	input_text_flags = 0;
	if (input_text_disable > 0)
	{
		input_text_disable--;
		input_text_flags = ImGuiInputTextFlags_ReadOnly;
	}

	ImGuizmo::SetGizmoSizeClipSpace(0.12f);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //ImGuiWindowFlags_MenuBar
	window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport;
	viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	// Check if the menu should be shown
	if (show_menu)
	{
		// Get screen size
		ImVec2 screen_size = ImGui::GetIO().DisplaySize;

		// Set the size and position of the ImGui window
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always); // Top-left corner
		ImGui::SetNextWindowSize(ImVec2(screen_size.x / 4.0f, screen_size.y), ImGuiCond_Always); // 1/4 of the screen width

		// Create a window without any decoration
		ImGui::Begin("##Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
/*		// Check if the menu should be shown
		if (show_menu)
		{
			LoadTexture(".. / Content / logo_small_copy.png");

			// Ensure the texture is loaded and valid
			if (my_image_texture.IsValid())
			{
				// Display the image
				ImGui::Image(my_imgui_texture, ImVec2(512, 512)); // Adjust the size as needed
				// Draw the image using Wicked Engine
				//wi::image::Draw(&my_image_texture, wi::image::Params(75, 40, image_size.x, image_size.y), cmd);
			}
			else
			{
				// Handle texture load failure
				//wi::image::Draw(wi::texturehelper::getLogo(), wi::image::Params(75, 40, image_size.x, image_size.y), cmd);
			}
		}
*/
		// Set background color (optional)
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Dark gray background

		// Calculate the size of each button
		ImVec2 button_size(300, 50); // Adjust the size as needed

		// Calculate the total height of the buttons including spacing
		float spacing = ImGui::GetStyle().ItemSpacing.y;
		float total_button_height = 3 * button_size.y + 2 * spacing;
		float total_content_height = total_button_height + image_size.y + spacing; // Include the image height

		// Calculate the starting position to center the buttons vertically
//		float start_y = (ImGui::GetWindowSize().y - total_content_height) / 2;
	//	float start_x = (ImGui::GetWindowSize().x - button_size.x) / 2;

		// Calculate starting positions
		float start_x = (ImGui::GetWindowSize().x - button_size.x) / 2; // Center X for buttons
		float start_y = (ImGui::GetWindowSize().y - total_content_height) / 2; // Center Y for all content

		// Move cursor position below the image
		ImGui::SetCursorPos(ImVec2(start_x, start_y));
		
		// Draw the image using Wicked Engine
		ImGui::Image(&my_image_texture, image_size); // Display the image

		//wi::image::Draw(&my_image_texture, wi::image::Params(10, 20, image_size.x, image_size.y), cmds);

		// Draw each button
		ImGui::SetCursorPos(ImVec2(start_x, ImGui::GetCursorPosY()));
		if (ImGui::Button("Start Game", button_size))
		{
			SwitchScene("../Content/models/sponza/sponza.wiscene"); // Load the game scene
		}

		ImGui::SetCursorPos(ImVec2(start_x, ImGui::GetCursorPosY() + button_size.y + spacing));
		if (ImGui::Button("Options", button_size))
		{
			show_menu = false; // Set the flag to false to indicate exiting the menu
			show_options = true; // Set the flag to true to indicate the options

			SwitchScene("../Content/models/cylinder.wiscene"); // Load the options scene
		}

		ImGui::SetCursorPos(ImVec2(start_x, ImGui::GetCursorPosY() + button_size.y + spacing));
		if (ImGui::Button("Exit", button_size))
		{
			// Perform necessary cleanup and exit
			std::exit(0); // Exit the application
		}

		ImGui::PopStyleColor(); // Restore the original style
		ImGui::End();

	}

	// Check if the menu should be shown
	if (show_options)
	{

	}

/*
	// Get screen size
	ImVec2 screen_size = ImGui::GetIO().DisplaySize;

	// Set the size and position of the ImGui window
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always); // Top-left corner
	ImGui::SetNextWindowSize(ImVec2(screen_size.x / 4.0f, screen_size.y), ImGuiCond_Always); // 1/4 of the screen width

	// Create a window without any decoration
	ImGui::Begin("##Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Set background color (optional)
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Dark gray background

	// Calculate the size of each button
	ImVec2 button_size(150, 50); // Adjust the size as needed

	// Calculate the total height of the buttons including spacing
	float spacing = ImGui::GetStyle().ItemSpacing.y;
	float total_button_height = 3 * button_size.y + 2 * spacing;

	// Calculate the starting position to center the buttons vertically
	float start_y = (ImGui::GetWindowSize().y - total_button_height) / 2;
	float start_x = (ImGui::GetWindowSize().x - button_size.x) / 2;

	// Set the cursor position to the calculated starting position and draw each button
	ImGui::SetCursorPos(ImVec2(start_x, start_y));
	if (ImGui::Button("Start Game", button_size))
	{
		SwitchScene("../Content/models/sponza/sponza.wiscene"); // Load the game scene
	}

	ImGui::SetCursorPos(ImVec2(start_x, start_y + button_size.y + spacing));
	if (ImGui::Button("Options", button_size))
	{
		SwitchScene("../Content/models/cylinder.wiscene"); // Load the options scene
	}

	ImGui::SetCursorPos(ImVec2(start_x, start_y + 2 * (button_size.y + spacing)));
	if (ImGui::Button("Exit", button_size))
	{
		show_menu = false; // Set the flag to false to indicate exiting the menu
	}

	ImGui::PopStyleColor(); // Restore the original style

	ImGui::End();*/

	bool dockingopen = false;
	if (ImGui::Begin("DockSpaceWicked", &dockingopen, window_flags))
	{
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		if (ImGui::DockBuilderGetNode(ImGui::GetID("MyDockspace")) == NULL || redock_windows_on_start)
		{
			//Default docking setup.
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // ImGuiDockNodeFlags_Dockspace); // Add empty node
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

			ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
			ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.18f, NULL, &dock_main_id);
			ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
			ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.208f, NULL, &dock_main_id); //0.208f . 0.24

			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_main_id);
			node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

			ImGui::DockBuilderDockWindow("Scene", dock_id_left);
			ImGui::DockBuilderDockWindow("Trees", dock_id_right);
			ImGui::DockBuilderDockWindow("Options", dock_id_right);
			ImGui::DockBuilderDockWindow(ICON_MD_TEXT_SNIPPET " Wicked Backlog", dock_id_bottom);
			ImGui::DockBuilderDockWindow(ICON_MD_BUG_REPORT " Debugger", dock_id_bottom);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 vOldWindowMinSize = style.WindowMinSize;
		style.WindowMinSize.x = 150.0f;

		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.25f), ImGuiDockNodeFlags_None);

		style.WindowMinSize = vOldWindowMinSize;

		//PE: Attach widget to docking host window.
		if (highlight_entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(highlight_entity);
			if (object != nullptr)
			{
				//PE: Need to disable physics while editing.
				wi::physics::SetSimulationEnabled(false);

				TransformComponent* obj_tranform = scene.transforms.GetComponent(highlight_entity);

				if (obj_tranform != nullptr)
				{
					XMFLOAT4X4 obj_world;

					XMStoreFloat4x4(&obj_world, obj_tranform->GetLocalMatrix());

					float fCamView[16];
					float fCamProj[16];
					DirectX::XMStoreFloat4x4((XMFLOAT4X4*)&fCamView[0], camera.GetView());
					DirectX::XMStoreFloat4x4((XMFLOAT4X4*)&fCamProj[0], camera.GetProjection());

					ImGuiIO& io = ImGui::GetIO();
					ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

					//PE: Generate real docking host window name.
					ImDrawList* host_drawlist = NULL;
					ImGuiWindow* host_window = ImGui::FindWindowByName("DockSpaceWicked");
					ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
					char title[256];
					ImFormatString(title, IM_ARRAYSIZE(title), "%s/DockSpace_%08X", host_window->Name, dockspace_id);
					host_window = ImGui::FindWindowByName(title);
					if (host_window)
						ImGuizmo::SetDrawlist(host_window->DrawList);
					else
						ImGuizmo::SetDrawlist();

					ImGuizmo::SetOrthographic(false);


					//PE: TODO - if mesh has pivot at center like sponza, use object aabb.getCenter(); as start matrix obj_world._41,obj_world._42,obj_world._43.
					//const wi::primitive::AABB& aabb = *scene.aabb_objects.GetComponent(highlight_entity);
					//XMFLOAT3 center = aabb.getCenter();

					ImGuizmo::Manipulate(fCamView, fCamProj, mCurrentGizmoOperation, mCurrentGizmoMode, &obj_world._11, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);

					if (ImGuizmo::IsUsing())
					{
						//PE: Use matrix directly. (rotation_local is using quaternion from XMQuaternionRotationMatrix)
						obj_tranform->world = (XMFLOAT4X4)&obj_world._11;
						obj_tranform->ApplyTransform();

						//PE: Decompose , can give some matrix->euler problems. just left here for reference.
						//float matrixTranslation[3], matrixRotation[3], matrixScale[3];
						//ImGuizmo::DecomposeMatrixToComponents(&obj_world._11, matrixTranslation, matrixRotation, matrixScale);
						//obj_tranform->ClearTransform();
						//obj_tranform->Translate((const XMFLOAT3&)matrixTranslation[0]);
						//obj_tranform->RotateRollPitchYaw((const XMFLOAT3&)matrixRotation[0]);
						//obj_tranform->Scale((const XMFLOAT3&)matrixScale[0]);
						//obj_tranform->UpdateTransform();

						bIsUsingWidget = true;
					}

					//PE: Other helpfull functions.
					//ImGuizmo::DrawGrid(fCamView, fCamProj, identityMatrix, 100.f);
					//ImGuizmo::DrawCubes(fCamView, fCamProj, &obj_world._11, 1);
					//ImGuizmo::ViewManipulate(fCamView, 1000.0, ImVec2(io.DisplaySize.x - 528, ImGui::GetWindowPos().y), ImVec2(128, 128), 0x10101010);

				}
			}
		}
		ImGui::End();
	}

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	if (show_performance_data)
		DisplayPerformanceData(&show_performance_data);

	ImGui::Begin("Options");
	{
		bool bSettingsOpen = true;
		if (ImGui::CollapsingHeader(ICON_MD_SETTINGS "  Settings", ImGuiTreeNodeFlags_None)) //ImGuiTreeNodeFlags_DefaultOpen
		{
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));

			static bool bVSync = true;
			if (ImGui::Checkbox("VSync", &bVSync))
			{
				wi::eventhandler::SetVSync(bVSync);
			}

			bool bPhysics = wi::physics::IsSimulationEnabled();
			if (ImGui::Checkbox("Physics On", &bPhysics))
			{
				wi::physics::SetSimulationEnabled(bPhysics);
			}

			static bool bEyeAdaption = active_render->getEyeAdaptionEnabled();
			if (ImGui::Checkbox("Eye Adaption", &bEyeAdaption))
			{
				active_render->setEyeAdaptionEnabled(bEyeAdaption);
			}

			static bool bDrawGrid = false;
			if (ImGui::Checkbox("Draw Grid", &bDrawGrid))
			{
				wi::renderer::SetToDrawGridHelper(bDrawGrid);
			}
			static bool bTemporalAAEnabled = true;
			if (ImGui::Checkbox("Temporal AA", &bTemporalAAEnabled))
			{
				wi::renderer::SetTemporalAAEnabled(bTemporalAAEnabled);
			}
			static bool bFXAAEnabled = true;
			if (ImGui::Checkbox("FXAA", &bFXAAEnabled))
			{
				setFXAAEnabled(bFXAAEnabled);
			}
			static bool bVoxelGIEnabled = false;
			if (ImGui::Checkbox("Voxel GI", &bVoxelGIEnabled))
			{
				wi::renderer::SetVXGIEnabled(bVoxelGIEnabled);
			}

			static bool bFSR = active_render->getFSREnabled();
			if (ImGui::Checkbox("FidelityFX FSR", &bFSR))
			{
				active_render->setFSREnabled(bFSR);
			}
			if (bFSR)
			{
				static float fFSRSharpness = active_render->getFSRSharpness();
				ImGui::PushItemWidth(-4);
				if (ImGui::SliderFloat("##FSRSharpness", &fFSRSharpness, 0.0, 2.0, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				{
					active_render->setFSRSharpness(fFSRSharpness);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("FSR Sharpness");
				ImGui::PopItemWidth();


				static float fResolutionScale = active_render->resolutionScale;
				ImGui::PushItemWidth(-4);
				if (ImGui::SliderFloat("##fResolutionScale", &fResolutionScale, 0.25, 2.0, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				{
					active_render->resolutionScale = fResolutionScale;
					ResizeBuffers();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resolution Scale");
				ImGui::PopItemWidth();
			}
		}

		if (ImGui::CollapsingHeader(ICON_MD_SCREENSHOT_MONITOR "  Camera Screenshot", ImGuiTreeNodeFlags_None)) //ImGuiTreeNodeFlags_DefaultOpen
		{
			static std::vector<MaterialComponent::TextureMap> screenshots;

			Texture* lpTexture = (Texture*)lastPostprocessRT;
			if (lpTexture)
			{
				float img_width = ImGui::GetContentRegionAvail().x - 2;
				float iheight = (float)lpTexture->desc.height;
				float iwidth = (float)lpTexture->desc.width;
				float height_ratio = iheight / iwidth;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::ImageButton(lpTexture, ImVec2(img_width, img_width * height_ratio));
				ImGui::PopStyleVar();
			}
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5f) - (IMGUIBUTTONWIDE * 0.5f), 0.0f));
			if (ImGui::Button("Take Screenshot", ImVec2(IMGUIBUTTONWIDE, 0)))
			{
				std::string name = "scr" + std::to_string(screenshots.size()) + ".jpg";
				wi::helper::screenshot(myswapChain, name);

				MaterialComponent::TextureMap tm;
				tm.resource = wi::resourcemanager::Load(name);
				tm.name = name;
				screenshots.push_back(tm);
			}
			if (!screenshots.empty())
			{
				for (int i = 0; i < screenshots.size(); i++)
				{
					if (!screenshots[i].name.empty())
					{
						Texture* lpTex = (Texture*)screenshots[i].GetGPUResource();
						float img_width = ImGui::GetContentRegionAvail().x - 2;
						float iheight = (float)lpTex->desc.height;
						float iwidth = (float)lpTex->desc.width;
						float height_ratio = iheight / iwidth;
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
						ImGui::ImageButton(lpTex, ImVec2(img_width, img_width * height_ratio));
						ImGui::PopStyleVar();
					}
				}
			}
		}

		if (ImGui::CollapsingHeader(ICON_MD_HIGHLIGHT_ALT "  Vegitation Editor", ImGuiTreeNodeFlags_None)) //ImGuiTreeNodeFlags_DefaultOpen
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			ImGui::PopStyleVar();

			igTextTitle("Tree Editor");

			static bool bIsWireRender = false;
			if (ImGui::Checkbox("IsWireRender", &bIsWireRender))
			{
				wi::renderer::SetWireRender(bIsWireRender);
			}
			/*
			static bool bcEyeAdaption = active_render->getEyeAdaptionEnabled();
			if (ImGui::Checkbox("Eye Adaption", &bcEyeAdaption))
			{
				active_render->setEyeAdaptionEnabled(bcEyeAdaption);
			}
*/
			if (ImGui::Button("Load"))
			{
				std::string filePath, selectedFile;
				if (filemanager.OpenFileDialog(filePath, selectedFile))
				{
					generations = loadGenerationsFromFile(selectedFile);

					wi::backlog::post("Selected File Path: " + filePath + "\n" + "Selected File Name: " + selectedFile, wi::backlog::LogLevel::Default);

					if (!generations.empty())
					{
						std::string treename = "Tree";
						treeRenderer.CreateTree(scene, treename, generations);

						wi::backlog::post("L-system loaded and rendered", wi::backlog::LogLevel::Default);
					}
				}
				else
				{
					wi::backlog::post("File selection canceled or failed.", wi::backlog::LogLevel::Error);
				}
			}

			if (ImGui::Button("Save")) {
				std::string filePath;
				std::wstring saveFile = (L"treee.txt");
				if (filemanager.SaveFileDialog(filePath, saveFile)) {

					treeRenderer.SaveTree(generations, "treeee.txt");

					wi::backlog::post("Save File Path: " + filePath + "\n" + "Save File Name: ", wi::backlog::LogLevel::Default);
				}
				else {
					wi::backlog::post("File save canceled or failed.", wi::backlog::LogLevel::Error);
				}
			}

			if (ImGui::Button("Start")) {
				timesim.start(); // Start the time simulation
			}
			if (ImGui::Button("Stop")) {
				timesim.stop();
			}
			if (ImGui::Button("Reset")) {
				timesim.reset();
			}
			if (ImGui::Button("Reverse")) {
				timesim.reverse();
			}
			/*
			if (ImGui::Button("Pause")) {
			}
					*/
			timesim.update(); // Update the time simulation

			if (timesim.getIsReversed()) {
				//std::cout << "Reversing time for the remaining half of the simulation" << std::endl;
				//simulateNegativeGrowth(generations, timesim.getElapsedSeconds());
			}
			else
			{
				//simulateGrowth(generations, timesim.getElapsedSeconds());
			}
		}

		/*
				//PE: stop flickering when scrollbar goes on/off.
				if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
					ImGui::Text("");
					ImGui::Text("");
				}
		*/
		ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
		if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
		{
			imgui_got_focus = true;
		}
		if (ImGui::IsAnyItemFocused())
		{
			imgui_got_focus = true;
		}
	}
	ImGui::End();

	if (!imgui_got_focus_last)
	{
		//PE: Right mouse camera rotation.

		/*
		//PE: ImGui way to control camera.
		if (ImGui::IsMouseDown(1) && ImGui::IsMouseDragging(1))
		{
			float speed = 6.0f;
			float xdiff = imgui_io.MouseDelta.x / speed;
			float ydiff = imgui_io.MouseDelta.y / speed;
			camera_ang[0] += ydiff;
			camera_ang[1] += xdiff;
			if (camera_ang[0] > 180.0f)  camera_ang[0] -= 360.0f;
			if (camera_ang[0] < -89.999f)  camera_ang[0] -= 89.999f;
			if (camera_ang[0] > 89.999f)  camera_ang[0] = 89.999f;
		}
		*/

		//PE: Wicked way to control camera.
		static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
		static bool camControlStart = true;
		if (camControlStart)
		{
			originalMouse = wi::input::GetPointer();
		}

		if (wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE) || wi::input::Down(wi::input::MOUSE_BUTTON_RIGHT))
		{
			camControlStart = false;
			// Mouse delta from hardware read:
			float xDif = wi::input::GetMouseState().delta_position.x;
			float yDif = wi::input::GetMouseState().delta_position.y;
			xDif = 0.1f * xDif;
			yDif = 0.1f * yDif;
			wi::input::SetPointer(originalMouse);
			wi::input::HidePointer(true);
			camera_ang[0] += yDif;
			camera_ang[1] += xDif;
			if (camera_ang[0] < -89.999f)  camera_ang[0] = -89.999f;
			if (camera_ang[0] > 89.999f)  camera_ang[0] = 89.999f;
		}
		else
		{
			camControlStart = true;
			wi::input::HidePointer(false);
		}

		float movespeed = CAMERAMOVESPEED;
		
		if (imgui_io.KeyShift)
		{
			movespeed *= 3.0; //Speed up camera.
		}

		movespeed *= dt;

		if ((ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))) //W UP
		{
			camera_pos[0] += movespeed * camera.At.x;
			camera_pos[1] += movespeed * camera.At.y;
			camera_pos[2] += movespeed * camera.At.z;
		}
		if ((ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))) //S Down
		{
			camera_pos[0] += -movespeed * camera.At.x;
			camera_pos[1] += -movespeed * camera.At.y;
			camera_pos[2] += -movespeed * camera.At.z;
		}

		XMFLOAT3 dir_right;
		DirectX::XMStoreFloat3(&dir_right, camera.GetRight());

		if ((ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow))) //D Right
		{
			camera_pos[0] += -movespeed * dir_right.x;
			camera_pos[1] += -movespeed * dir_right.y;
			camera_pos[2] += -movespeed * dir_right.z;
		}
		if ((ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))) //A Left
		{
			camera_pos[0] += movespeed * dir_right.x;
			camera_pos[1] += movespeed * dir_right.y;
			camera_pos[2] += movespeed * dir_right.z;
		}
	}

	camera_transform.ClearTransform();
	camera_transform.Translate(XMFLOAT3(camera_pos[0], camera_pos[1], camera_pos[2]));
	camera_transform.RotateRollPitchYaw(XMFLOAT3(camera_ang[0] / (float) DIVTWOPI, camera_ang[1] / (float) DIVTWOPI, camera_ang[2] / (float) DIVTWOPI));
	camera_transform.UpdateTransform();
	camera.TransformCamera(camera_transform);
	camera.UpdateCamera();

	if (redock_windows_on_start)
	{
		redock_windows_on_start = false;
		ImGui::SetWindowFocus("Scene");
	}

	imgui_got_focus_last = imgui_got_focus;

	RenderPath3D::Update(dt);
}

void Example_ImGuiRenderer::DisplayPerformanceData(bool* p_open)
{
	const float DISTANCE = 6.0f;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 window_pos = ImVec2( viewport->Pos.x + (viewport->Size.x * 0.5f) + DISTANCE, viewport->Pos.y + DISTANCE);
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f,0.0f) );
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	if (ImGui::Begin("##DisplayPerformanceData", p_open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("Wicked Engine (%s) - FPS: %.1f", wi::version::GetVersionString(),ImGui::GetIO().Framerate);
	}
	ImGui::End();
}

void Example_ImGuiRenderer::igTextTitle(const char* text)
{
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();
	float w = ImGui::GetContentRegionAvail().x;
	if (customfontlarge) ImGui::PushFont(customfontlarge);
	float textwidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5f) - (textwidth * 0.5f) - (window->DC.Indent.x * 0.5f), 0.0f));
	ImGui::Text("%s", text);
	if (customfontlarge) ImGui::PopFont();
}

void add_my_font(const char *fontpath)
{
	//PE: Add all lang.
	static const ImWchar Generic_ranges_everything[] =
	{
	   0x0020, 0xFFFF, // Everything test.
	   0,
	};
	static const ImWchar Generic_ranges_most_needed[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0100, 0x017F,	//0100 — 017F  	Latin Extended-A
		0x0180, 0x024F,	//0180 — 024F  	Latin Extended-B
		0,
	};

	float FONTUPSCALE = 1.0; //Font upscaling.
	float FontSize = 15.0f * font_scale;
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();

	customfont = io.Fonts->AddFontFromFileTTF(fontpath, FontSize * FONTUPSCALE, NULL, &Generic_ranges_everything[0]); //Set as default font.
	if (customfont)
	{
		#ifdef INCLUDEICONFONT
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphOffset = ImVec2(0, 3);
		//config.GlyphMinAdvanceX = FontSize * FONTUPSCALE; // Use if you want to make the icon monospaced
		static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };
		io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_MD, FontSize * FONTUPSCALE, &config, icon_ranges);
		#endif

		io.FontGlobalScale = 1.0f / FONTUPSCALE;
		customfontlarge = io.Fonts->AddFontFromFileTTF(fontpath, 20 * font_scale * FONTUPSCALE, NULL, &Generic_ranges_most_needed[0]); //Set as default font.
		if (!customfontlarge)
		{
			customfontlarge = customfont;
		}

		#ifdef INCLUDEICONFONT
		config.MergeMode = true;
		config.GlyphOffset = ImVec2(0, 2);
		//config.GlyphMinAdvanceX = 20 * font_scale * FONTUPSCALE; // Use if you want to make the icon monospaced
		io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_MD, FontSize * FONTUPSCALE, &config, icon_ranges);
		#endif

	}
	else
	{
		customfont = io.Fonts->AddFontDefault();
		customfontlarge = io.Fonts->AddFontDefault();
	}

	defaultfont = io.Fonts->AddFontDefault();
}

void style_dark_ruda( void )
{
	// Dark Ruda style by Raikiri from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 3.0;
	style.DisabledAlpha = 0.6000000238418579;
	style.WindowPadding = ImVec2(8.0, 8.0);
	style.WindowRounding = 0.0;
	style.WindowBorderSize = 1.0;
	style.WindowMinSize = ImVec2(32.0, 32.0);
	style.WindowTitleAlign = ImVec2(0.0, 0.5);
	style.WindowMenuButtonPosition = ImGuiDir_Up;
	style.ChildRounding = 0.0;
	style.ChildBorderSize = 1.0;
	style.PopupRounding = 0.0;
	style.PopupBorderSize = 1.0;
	style.FramePadding = ImVec2(4.0, 4.0);
	style.FrameRounding = 2.0;
	style.FrameBorderSize = 1.0;
	style.ItemSpacing = ImVec2(4.0, 4.0);
	style.ItemInnerSpacing = ImVec2(4.0, 4.0);
	style.CellPadding = ImVec2(10.0, 10.0);
	style.IndentSpacing = 21.0;
	style.ColumnsMinSpacing = 6.0;
	style.ScrollbarSize = 14.0;
	style.ScrollbarRounding = 9.0;
	style.GrabMinSize = 10.0;
	style.GrabRounding = 4.0;
	style.TabRounding = 4.0;
	style.TabBorderSize = 0.0;
	style.TabMinWidthForCloseButton = 0.0;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5, 0.5);
	style.SelectableTextAlign = ImVec2(0.0, 0.0);

	style.Colors[ImGuiCol_Text] = ImVec4(0.9490196108818054f, 0.95686274766922f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.3568627536296844f, 0.4196078479290009f, 0.4666666686534882f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1098039224743843f, 0.1490196138620377f, 0.168627455830574f, 0.9f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1490196138620377f, 0.1764705926179886f, 0.2196078449487686f, 0.7f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.0784313753247261f, 0.09803921729326248f, 0.1176470592617989f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2470588237047195f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1176470592617989f, 0.2000000029802322f, 0.2784313857555389f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.08627451211214066f, 0.1176470592617989f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08627451211214066f, 0.1176470592617989f, 0.1372549086809158f, 0.6499999761581421f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0784313753247261f, 0.09803921729326248f, 0.1176470592617989f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1490196138620377f, 0.1764705926179886f, 0.2196078449487686f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.3899999856948853f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2000000029802322f, 0.2470588237047195f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1764705926179886f, 0.2196078449487686f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.08627451211214066f, 0.2078431397676468f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2784313857555389f, 0.5568627715110779f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.2784313857555389f, 0.5568627715110779f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3686274588108063f, 0.6078431606292725f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.2000000029802322f, 0.2470588237047195f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2784313857555389f, 0.5568627715110779f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(53.0f / 255.0f, 83.0f / 255.0f, 108.0f / 255.0f, 140.0f / 255.0f); // ImVec4(0.2000000029802322, 0.2470588237047195, 0.2862745225429535, 0.550000011920929);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.2000000029802322f, 0.2470588237047195f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.25f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.1098039224743843f, 0.1490196138620377f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);

	style.Colors[ImGuiCol_TabActive] = ImVec4(0.2000000029802322f, 0.2470588237047195f, 0.2862745225429535f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1098039224743843f, 0.1490196138620377f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1098039224743843f, 0.1490196138620377f, 0.168627455830574f, 1.0f);

	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);

	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);

	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

void SwitchScene(const std::string& scene_file)
{
	// Get the file extension from the scene_file
	std::string ext = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(scene_file));

	// Clear the current scene
	wi::scene::GetScene().Clear();

	// Check the file extension and load the scene accordingly
	if (ext == "GLB") // binary GLTF
	{
		Scene new_scene;
		ImportModel_GLTF(scene_file, new_scene); // Load the new scene
		wi::scene::GetScene().Merge(new_scene); // Merge it with the current scene
	}
	else
	{
		// Load other model formats directly
		wi::scene::LoadModel(scene_file);
	}

	// Optionally, update internal state or perform additional actions
}

void LoadTexture(const std::string& file_path)
{
	my_image_resource = wi::resourcemanager::Load(file_path);
	if (my_image_resource.IsValid())
	{
		my_image_texture = my_image_resource.GetTexture();

	}
	else
	{
		// Handle error: texture not loaded
		printf("Failed to load texture: %s\n", file_path.c_str());
	}
}

//void RenderWithWickedEngine(wi::graphics::CommandList cmd)
//{
