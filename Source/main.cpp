#include "Leadwerks.h"
using namespace Leadwerks;

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

// WinAPI Code
void InitWinAPI()
{
	auto window = Window::GetCurrent();
	if (window == NULL) return;

	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = window->GetHandle();

	if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)
	{
		Print("Error: Failed to register raw input devices.");
	}
}

static Vec2 rawmouseaxis = Vec2(0);
LRESULT CALLBACK ProcInputWin32(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INPUT:
	{
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			rawmouseaxis.x += float(raw->data.mouse.lLastX);
			rawmouseaxis.y += float(raw->data.mouse.lLastY);
		}
		break;
	}
	default:
		break;
	}

	return Leadwerks::WndProc(hWnd, message, wParam, lParam);
}

Vec2 GetMouseAxis(const float dpi = 1000)
{
	Vec2 t = Vec2(0);
	t.x = rawmouseaxis.x / dpi;
	t.y = rawmouseaxis.y / dpi;
	return t;
}

// Actors
class CameraControls : public Actor
{
public:
	bool freelookstarted = false;
	float mousesmoothing = 0.0f;
	float mouselookspeed = 1.0f;
	float movespeed = 4.0f;
	Vec2 freelookmousepos;
	Vec3 freelookrotation;
	Vec2 lookchange;

	bool rawmouse = false;

	void RawMouseLook()
	{
		if (!freelookstarted)
		{
			freelookstarted = true;
			freelookrotation = entity->GetRotation(true);
			freelookmousepos = GetMouseAxis();
		}
		auto newmousepos = GetMouseAxis();
		lookchange.x = lookchange.x * mousesmoothing + (newmousepos.y - freelookmousepos.y) * 100.0f * mouselookspeed * (1.0f - mousesmoothing);
		lookchange.y = lookchange.y * mousesmoothing + (newmousepos.x - freelookmousepos.x) * 100.0f * mouselookspeed * (1.0f - mousesmoothing);
		if (Math::Abs(lookchange.x) < 0.001f) lookchange.x = 0.0f;
		if (Math::Abs(lookchange.y) < 0.001f) lookchange.y = 0.0f;
		if (lookchange.x != 0.0f or lookchange.y != 0.0f)
		{
			freelookrotation.x += lookchange.x;
			freelookrotation.y += lookchange.y;
			Print(freelookrotation.ToString());
			entity->SetRotation(freelookrotation, true);
		}
		freelookmousepos = newmousepos;
	}

	void RelativeMouseLook()
	{
		auto window = Window::GetCurrent();
		auto context = Context::GetCurrent();
		auto cx = Math::Round(context->GetWidth() / 2);
		auto cy = Math::Round(context->GetHeight() / 2);
		auto mpos = window->GetMousePosition();
		window->SetMousePosition(cx, cy);
		auto centerpos = window->GetMousePosition();

		if (freelookstarted)
		{
			float looksmoothing = Math::Clamp(mousesmoothing, 0.5f, 1.0f); //mousesmoothing; //0.5f;
			float lookspeed = mouselookspeed / 10.0f;

			mpos.x = mpos.x * looksmoothing + freelookmousepos.x * (1 - looksmoothing);
			mpos.y = mpos.y * looksmoothing + freelookmousepos.y * (1 - looksmoothing);

			auto dx = (mpos.x - centerpos.x) * lookspeed;
			auto dy = (mpos.y - centerpos.y) * lookspeed;

			freelookrotation.x = freelookrotation.x + dy;
			freelookrotation.y = freelookrotation.y + dx;
			entity->SetRotation(freelookrotation, true);
			freelookmousepos = mpos.xy();
		}
		else
		{
			freelookstarted = true;
			freelookrotation = entity->GetRotation(true);
			freelookmousepos = window->GetMousePosition().xy();
		}

#if 0
		auto window = Window::GetCurrent();
		auto context = Context::GetCurrent();
		auto cx = Math::Round(context->GetWidth() / 2);
		auto cy = Math::Round(context->GetHeight() / 2);

		if (!freelookstarted)
		{
			freelookstarted = true;
			freelookrotation = entity->GetRotation(true);
			freelookmousepos = Vec2(cx, cy);
		}

		auto newmousepos = Vec2((window->GetMousePosition().x - cx) / 1000, (window->GetMousePosition().y - cy) / 1000);
		lookchange.x = lookchange.x * mousesmoothing + (newmousepos.y - freelookmousepos.y) * 100.0f * mouselookspeed * (1.0f - mousesmoothing);
		lookchange.y = lookchange.y * mousesmoothing + (newmousepos.x - freelookmousepos.x) * 100.0f * mouselookspeed * (1.0f - mousesmoothing);

		if (Math::Abs(lookchange.x) < 0.001f) lookchange.x = 0.0f;
		if (Math::Abs(lookchange.y) < 0.001f) lookchange.y = 0.0f;
		if (lookchange.x != 0.0f or lookchange.y != 0.0f)
		{
			freelookrotation.x += lookchange.x;
			freelookrotation.y += lookchange.y;
			Print(freelookrotation.ToString());
			entity->SetRotation(freelookrotation, true);
		}
		freelookmousepos = newmousepos;
#endif
	}

	virtual void UpdateWorld()
	{
		auto window = Window::GetCurrent();
		if (window == NULL) return;

		if (rawmouse)
			RawMouseLook();
		else
			RelativeMouseLook();

		float speed = movespeed / 60.0f;
		if (window->KeyDown(Key::Shift))
		{
			speed *= 10.0f;
		}
		else if (window->KeyDown(Key::Control))
		{
			speed *= 0.25f;
		}
		if (window->KeyDown(Key::E)) entity->Translate(0, speed, 0);
		if (window->KeyDown(Key::Q)) entity->Translate(0, -speed, 0);
		if (window->KeyDown(Key::D)) entity->Move(speed, 0, 0);
		if (window->KeyDown(Key::A)) entity->Move(-speed, 0, 0);
		if (window->KeyDown(Key::W)) entity->Move(0, 0, speed);
		if (window->KeyDown(Key::S)) entity->Move(0, 0, -speed);
	}
};

class Spinner : public Actor
{
public:
	virtual void UpdateWorld()
	{
		entity->Turn(0, Leadwerks::Time::GetSpeed() * 0.45f, 0);
	}
};

// Main App 
int main(int argc,const char *argv[])
{
	// Create a window
	auto window = Window::Create("Leadwerks", 0, 0, 1280, 720, Window::Titlebar | Window::Center);

	// WinAPI
	InitWinAPI();

	// Replace the engine's proc with ours.
	WNDPROC OldProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window->GetHandle(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ProcInputWin32)));

	// Create a frame buffer
	auto framebuffer = Context::Create(window);

	// Create world
	auto world = World::Create();

	//Create a camera
	auto camera = Camera::Create();
	camera->SetClearColor(0.125);
	camera->SetFOV(70);
	camera->SetPosition(0, 0, -3);

	//Create a light
	auto light = DirectionalLight::Create();
	light->SetRotation(35, 45, 0);
	light->SetRange(-10, 10);

	//Create a box
	auto box = Model::Box();
	box->SetColor(0, 0, 1, 1);

	//Attach actors;
	auto box_actor = new Spinner();
	box->SetActor(box_actor);

	auto camera_actor = new CameraControls();
	camera->SetActor(camera_actor);
	if (camera_actor->rawmouse == true)
		Print("Raw Mouse");
	else
		Print("Relative Mouse");

	while (window->Closed() == false and window->KeyDown(Key::Escape) == false)
	{
		if (window->KeyHit(Key::F1))
		{
			camera_actor->rawmouse = !camera_actor->rawmouse;
			if (camera_actor->rawmouse == true) 
				Print("Raw Mouse");
			else
				Print("Relative Mouse");
		}

		Time::Update();
		world->Update();
		world->Render();

		framebuffer->Sync();
	}
}