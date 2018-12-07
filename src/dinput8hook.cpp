#include "dinput8hook.h"

#define DIRECTINPUT_VERSION 0x0800
#include "win32.h"
#include <algorithm>
#include <cstring>
#include <dinput.h>
#include <sstream>
#include <windows.h>

static const GUID GUID_SysMouse
    = {0x6F1D2B60,
       0xD5A0,
       0x11CF,
       {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
static const GUID GUID_SysKeyboard
    = {0x6F1D2B61,
       0xD5A0,
       0x11CF,
       {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

enum DeviceType {
    Mouse,
    Keyboard,
};

enum ScrollClickState {
    MouseClick,
    MouseRelease,
};

static HINSTANCE dinput8_instance = NULL;

#define NUM_PROCS 6
static const char *const proc_names[NUM_PROCS] = {
    "DirectInput8Create", "DllCanUnloadNow",     "DllGetClassObject",
    "DllRegisterServer",  "DllUnregisterServer", "GetdfDIJoystick",
};
static FARPROC procs[NUM_PROCS] = {};
static uint8_t keyboard_state[256] = {};
static ScrollClickState scroll_click_state = MouseRelease;

class DirectInputDeviceDetour : public IDirectInputDevice8A
{
private:
    IDirectInputDevice8A *device_;
    DeviceType device_type_;
    ULONG num_refs_;

public:
    DirectInputDeviceDetour(IDirectInputDevice8A *device,
                            DeviceType device_type)
        : device_(device)
        , device_type_(device_type)
        , num_refs_(1)
    {
    }

    // IUnknown methods
    HRESULT __stdcall QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
        return device_->QueryInterface(riid, ppvObj);
    }

    ULONG __stdcall AddRef(void)
    {
        return ++num_refs_;
    }

    ULONG __stdcall Release(void)
    {
        if (--num_refs_ == 0) {
            device_->Release();
            delete this;
            return 0;
        } else {
            return num_refs_;
        }
    }

    // IDirectInputDevice8A methods
    HRESULT __stdcall GetCapabilities(LPDIDEVCAPS a)
    {
        return device_->GetCapabilities(a);
    }

    HRESULT __stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA a,
                                  LPVOID b,
                                  DWORD c)
    {
        return device_->EnumObjects(a, b, c);
    }

    HRESULT __stdcall GetProperty(REFGUID a, DIPROPHEADER *b)
    {
        return device_->GetProperty(a, b);
    }

    HRESULT __stdcall SetProperty(REFGUID a, const DIPROPHEADER *b)
    {
        return device_->SetProperty(a, b);
    }

    HRESULT __stdcall Acquire(void)
    {
        return device_->Acquire();
    }

    HRESULT __stdcall Unacquire(void)
    {
        return device_->Unacquire();
    }

    HRESULT __stdcall GetDeviceState(DWORD cbData, LPVOID lpvData)
    {
        if (device_type_ == Keyboard) {
            HRESULT result = device_->GetDeviceState(sizeof(keyboard_state),
                                                     keyboard_state);
            if (result != DI_OK) {
                return result;
            }
            std::memcpy(lpvData, keyboard_state,
                        std::min(cbData, (DWORD)sizeof(keyboard_state)));
            return DI_OK;
        } else {
            DIMOUSESTATE2 *state = reinterpret_cast<DIMOUSESTATE2 *>(lpvData);
            HRESULT result = device_->GetDeviceState(cbData, lpvData);
            if (result != DI_OK) {
                return result;
            }
            if (state->lZ == 0) {
                // Is not scrolling
                scroll_click_state = MouseRelease;
            } else {
                if (keyboard_state[DIK_LSHIFT] & 0x80) {
                    state->lZ = 0;
                    if (scroll_click_state == MouseClick) {
                        scroll_click_state = MouseRelease;
                    } else {
                        scroll_click_state = MouseClick;
                        state->rgbButtons[0] |= 0x80;
                    }
                }
            }
            return DI_OK;
        }
    }

    HRESULT __stdcall GetDeviceData(DWORD a,
                                    DIDEVICEOBJECTDATA *b,
                                    DWORD *c,
                                    DWORD d)
    {
        return device_->GetDeviceData(a, b, c, d);
    }

    HRESULT __stdcall SetDataFormat(const DIDATAFORMAT *a)
    {
        return device_->SetDataFormat(a);
    }

    HRESULT __stdcall SetEventNotification(HANDLE a)
    {
        return device_->SetEventNotification(a);
    }

    HRESULT __stdcall SetCooperativeLevel(HWND a, DWORD b)
    {
        return device_->SetCooperativeLevel(a, b);
    }

    HRESULT __stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA a,
                                    DWORD b,
                                    DWORD c)
    {
        return device_->GetObjectInfo(a, b, c);
    }

    HRESULT __stdcall GetDeviceInfo(LPDIDEVICEINSTANCEA a)
    {
        return device_->GetDeviceInfo(a);
    }

    HRESULT __stdcall RunControlPanel(HWND a, DWORD b)
    {
        return device_->RunControlPanel(a, b);
    }

    HRESULT __stdcall Initialize(HINSTANCE a, DWORD b, REFGUID c)
    {
        return device_->Initialize(a, b, c);
    }

    HRESULT __stdcall CreateEffect(REFGUID a,
                                   LPCDIEFFECT b,
                                   LPDIRECTINPUTEFFECT *c,
                                   LPUNKNOWN d)
    {
        return device_->CreateEffect(a, b, c, d);
    }

    HRESULT __stdcall EnumEffects(LPDIENUMEFFECTSCALLBACKA a, LPVOID b, DWORD c)
    {
        return device_->EnumEffects(a, b, c);
    }

    HRESULT __stdcall GetEffectInfo(LPDIEFFECTINFOA a, REFGUID b)
    {
        return device_->GetEffectInfo(a, b);
    }

    HRESULT __stdcall GetForceFeedbackState(LPDWORD a)
    {
        return device_->GetForceFeedbackState(a);
    }

    HRESULT __stdcall SendForceFeedbackCommand(DWORD a)
    {
        return device_->SendForceFeedbackCommand(a);
    }

    HRESULT __stdcall EnumCreatedEffectObjects(
        LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a,
        LPVOID b,
        DWORD c)
    {
        return device_->EnumCreatedEffectObjects(a, b, c);
    }

    HRESULT __stdcall Escape(LPDIEFFESCAPE a)
    {
        return device_->Escape(a);
    }

    HRESULT __stdcall Poll(void)
    {
        return device_->Poll();
    }

    HRESULT __stdcall SendDeviceData(DWORD a,
                                     LPCDIDEVICEOBJECTDATA b,
                                     LPDWORD c,
                                     DWORD d)
    {
        return device_->SendDeviceData(a, b, c, d);
    }

    HRESULT __stdcall EnumEffectsInFile(LPCSTR a,
                                        LPDIENUMEFFECTSINFILECALLBACK b,
                                        LPVOID c,
                                        DWORD d)
    {
        return device_->EnumEffectsInFile(a, b, c, d);
    }

    HRESULT __stdcall WriteEffectToFile(LPCSTR a,
                                        DWORD b,
                                        LPDIFILEEFFECT c,
                                        DWORD d)
    {
        return device_->WriteEffectToFile(a, b, c, d);
    }

    HRESULT __stdcall BuildActionMap(LPDIACTIONFORMATA a, LPCSTR b, DWORD c)
    {
        return device_->BuildActionMap(a, b, c);
    }

    HRESULT __stdcall SetActionMap(LPDIACTIONFORMATA a, LPCSTR b, DWORD c)
    {
        return device_->SetActionMap(a, b, c);
    }

    HRESULT __stdcall GetImageInfo(LPDIDEVICEIMAGEINFOHEADERA a)
    {
        return device_->GetImageInfo(a);
    }
};

class DirectInputDetour : public IDirectInput8A
{
private:
    IDirectInput8A *dinput_;
    ULONG num_refs_;

public:
    explicit DirectInputDetour(IDirectInput8A *dinput)
        : dinput_(dinput)
        , num_refs_(1)
    {
    }

    // IUnknown methods
    HRESULT __stdcall QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
        return dinput_->QueryInterface(riid, ppvObj);
    }

    ULONG __stdcall AddRef()
    {
        return ++num_refs_;
    }

    ULONG __stdcall Release()
    {
        if (--num_refs_ == 0) {
            dinput_->Release();
            delete this;
            return 0;
        } else {
            return num_refs_;
        }
    }

    // IDirectInput8A methods
    HRESULT __stdcall CreateDevice(REFGUID rguid,
                                   LPDIRECTINPUTDEVICE8A *lplpDirectInputDevice,
                                   LPUNKNOWN pUnkOuter)
    {
        if (rguid != GUID_SysKeyboard && rguid != GUID_SysMouse) {
            return dinput_->CreateDevice(rguid, lplpDirectInputDevice,
                                         pUnkOuter);
        }
        IDirectInputDevice8A *device;
        HRESULT result = dinput_->CreateDevice(rguid, &device, pUnkOuter);
        if (result != DI_OK) {
            return result;
        }
        DeviceType device_type;
        if (rguid == GUID_SysKeyboard) {
            device_type = Keyboard;
        } else {
            device_type = Mouse;
        }
        *lplpDirectInputDevice
            = new DirectInputDeviceDetour(device, device_type);
        return DI_OK;
    }

    HRESULT __stdcall EnumDevices(DWORD a,
                                  LPDIENUMDEVICESCALLBACKA b,
                                  void *c,
                                  DWORD d)
    {
        return dinput_->EnumDevices(a, b, c, d);
    }

    HRESULT __stdcall GetDeviceStatus(REFGUID a)
    {
        return dinput_->GetDeviceStatus(a);
    }

    HRESULT __stdcall RunControlPanel(HWND a, DWORD b)
    {
        return dinput_->RunControlPanel(a, b);
    }

    HRESULT __stdcall Initialize(HINSTANCE a, DWORD b)
    {
        return dinput_->Initialize(a, b);
    }

    HRESULT __stdcall FindDevice(REFGUID a, LPCSTR b, LPGUID c)
    {
        return dinput_->FindDevice(a, b, c);
    }

    HRESULT __stdcall EnumDevicesBySemantics(LPCSTR a,
                                             LPDIACTIONFORMATA b,
                                             LPDIENUMDEVICESBYSEMANTICSCBA c,
                                             void *d,
                                             DWORD e)
    {
        return dinput_->EnumDevicesBySemantics(a, b, c, d, e);
    }

    HRESULT __stdcall ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a,
                                       LPDICONFIGUREDEVICESPARAMSA b,
                                       DWORD c,
                                       void *d)
    {
        return dinput_->ConfigureDevices(a, b, c, d);
    }
};

// Create a hooked DirectInput instance
extern "C" HRESULT __stdcall DirectInput8Create_wrapper(HINSTANCE hinst,
                                                        DWORD dwVersion,
                                                        REFIID riidltf,
                                                        LPVOID *ppvOut,
                                                        LPUNKNOWN punkOuter)
{
    typedef HRESULT(__stdcall * DirectInput8Create_t)(
        HINSTANCE hinst, DWORD dwVersion, REFIID riidltf,
        IDirectInput8A * *ppvOut, LPUNKNOWN punkOuter);
    DirectInput8Create_t DirectInput8Create_real
        = reinterpret_cast<DirectInput8Create_t>(procs[0]);
    IDirectInput8A *dinput;
    HRESULT result = DirectInput8Create_real(hinst, dwVersion, riidltf, &dinput,
                                             punkOuter);
    if (result != DI_OK) {
        return result;
    }
    *reinterpret_cast<IDirectInput8A **>(ppvOut)
        = new DirectInputDetour(dinput);
    return DI_OK;
}

namespace dinput8hook
{
    void install()
    {
        std::wstring path = win32::get_system_directory() + L"\\dinput8.dll";
        if ((dinput8_instance = LoadLibraryW(path.c_str())) == NULL) {
            throw win32::get_last_error_exception();
        }
        for (int i = 0; i < NUM_PROCS; ++i) {
            procs[i] = GetProcAddress(dinput8_instance, proc_names[i]);
        }
    }

    void uninstall()
    {
        if (dinput8_instance != NULL) {
            FreeLibrary(dinput8_instance);
        }
    }
}

// Export actual dinput8.dll functions
#define FUNC(name, num)                                          \
    extern "C" __declspec(naked) void __stdcall name##_wrapper() \
    {                                                            \
        __asm { jmp procs[num * 4] }                             \
    }

FUNC(DllCanUnloadNow, 1)
FUNC(DllGetClassObject, 2)
FUNC(DllRegisterServer, 3)
FUNC(DllUnregisterServer, 4)
FUNC(GetdfDIJoystick, 5)
