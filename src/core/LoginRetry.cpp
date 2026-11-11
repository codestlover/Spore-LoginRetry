
#include <Windows.h>
#include "core/Name.hpp"
#include "core/Policy.hpp"
#include "core/Sites.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ModAPI {
typedef void (*InitFunction)();
__declspec(dllimport) void AddPostInitFunction(InitFunction);
__declspec(dllimport) void AddDisposeFunction(InitFunction);
}

namespace {
using LoginFn = void(__thiscall*)(void*);
using ResultFn = void(__thiscall*)(void*, short);
using ResponseFn = bool(__thiscall*)(void*, int, void*, void*);
using SubmitFn = void(__thiscall*)(void*, const char*, const char*, const wchar_t*);

struct Attempt {
    bool armed;
    unsigned retried;
    DWORD started;
    UINT_PTR timer;
    void* manager;
    short result;
};

struct Answer {
    bool known;
    int transport;
    int status;
};

struct Hook {
    uint32_t slot;
    uint32_t function;
    void* replacement;
    bool installed;
};

uintptr_t gameBase;
DWORD gameThread;
void** requests;
Attempt attempt{};
Answer answer{};
LoginFn gameLogin;
ResultFn gameResult;
ResponseFn gameResponse;
SubmitFn gameSubmit;
bool bareStatusLine;

void* At(uint32_t address) {
    return reinterpret_cast<void*>(gameBase + address - HostPreferredBase);
}

bool Readable(const void* p, size_t bytes) {
    uintptr_t cursor = reinterpret_cast<uintptr_t>(p), end = cursor + bytes;
    if (!p || end < cursor)
        return false;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &info, sizeof(info)) || info.State != MEM_COMMIT ||
            (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
            return false;
        uintptr_t next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (next <= cursor)
            return false;
        cursor = next;
    }
    return true;
}

void StopTimer() {
    if (attempt.timer)
        KillTimer(nullptr, attempt.timer);
    attempt.timer = 0;
}

const char* Text(void* manager, unsigned offset) {
    auto* text = reinterpret_cast<char**>(static_cast<char*>(manager) + offset);
    return text[0] && text[0] != text[1] && *text[0] ? text[0] : nullptr;
}

void Finish(void* manager, short result) {
    attempt.armed = false;
    attempt.retried = 0;
    gameResult(manager, result);
}

void CALLBACK OnTimer(HWND, UINT, UINT_PTR id, DWORD) {
    KillTimer(nullptr, id);
    if (id != attempt.timer)
        return;
    attempt.timer = 0;
    if (!attempt.armed)
        return;
    void* manager = attempt.manager;
    void* queue = *requests;
    const char* user = Text(manager, UserOffset);
    const char* password = Text(manager, PasswordOffset);
    if (!queue || !user || !password) {
        Finish(manager, attempt.result);
        return;
    }
    ++attempt.retried;
    gameSubmit(queue, user, password, nullptr);
}

void __fastcall OnLogin(void* manager, void*) {
    StopTimer();
    attempt = {true, 0, GetTickCount(), 0, manager, Policy::Success};
    answer = {};
    gameLogin(manager);
}

bool __fastcall OnResponse(void* job, void*, int transport, void* response, void* context) {
    int status = 0;
    if (response && Readable(static_cast<char*>(response) + StatusOffset, sizeof(status)))
        memcpy(&status, static_cast<char*>(response) + StatusOffset, sizeof(status));
    answer = {true, transport, status};
    return gameResponse(job, transport, response, context);
}

void __fastcall OnResult(void* manager, void*, short result) {
    Answer last = answer;
    answer = {};
    StopTimer();
    if (result != Policy::Success && attempt.armed && last.known && GetCurrentThreadId() == gameThread) {
        unsigned delay =
            Policy::Delay(result, last.transport, last.status, attempt.retried, GetTickCount() - attempt.started);
        UINT_PTR timer = delay ? SetTimer(nullptr, 0, delay, OnTimer) : 0;
        if (timer) {
            attempt.timer = timer;
            attempt.manager = manager;
            attempt.result = result;
            return;
        }
    }
    Finish(manager, result);
}

Hook hooks[] = {
    {LoginSlot, LoginFunction, reinterpret_cast<void*>(&OnLogin), false},
    {ResultSlot, ResultFunction, reinterpret_cast<void*>(&OnResult), false},
    {ResponseSlot, ResponseFunction, reinterpret_cast<void*>(&OnResponse), false},
};

bool HostMatches() {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(gameBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(gameBase + dos->e_lfanew);
    return nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 && nt->OptionalHeader.SizeOfImage == HostImageSize &&
           nt->FileHeader.TimeDateStamp == HostTimestamp;
}

bool CodeMatches() {
    for (const auto& s : kSignatures)
        if (!Readable(At(s.address), s.size) || memcmp(At(s.address), s.bytes, s.size))
            return false;
    for (const auto& s : kSlots)
        if (!Readable(At(s.address), sizeof(void*)) || *static_cast<void**>(At(s.address)) != At(s.function))
            return false;
    return true;
}

bool FindRequests() {
    uintptr_t global = 0;
    memcpy(&global, static_cast<char*>(At(RequestsFunction)) + 1, sizeof(global));
    if (global < gameBase || global - gameBase > HostImageSize - sizeof(void*) || global % sizeof(void*) ||
        !Readable(reinterpret_cast<void*>(global), sizeof(void*)))
        return false;
    requests = reinterpret_cast<void**>(global);
    return true;
}

bool Write(uint32_t slot, void* value) {
    auto* target = static_cast<void**>(At(slot));
    DWORD old = 0;
    if (!VirtualProtect(target, sizeof(void*), PAGE_READWRITE, &old))
        return false;
    *target = value;
    VirtualProtect(target, sizeof(void*), old, &old);
    return true;
}

bool WriteCode(uint32_t address, unsigned char value) {
    auto* target = static_cast<unsigned char*>(At(address));
    DWORD old = 0;
    if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &old))
        return false;
    *target = value;
    VirtualProtect(target, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), target, 1);
    return true;
}

void Uninstall() {
    if (bareStatusLine && *static_cast<unsigned char*>(At(StatusLineLength)) == StatusLineBare)
        WriteCode(StatusLineLength, StatusLineWithPhrase);
    bareStatusLine = false;
    for (auto& h : hooks) {
        if (h.installed && *static_cast<void**>(At(h.slot)) == h.replacement)
            Write(h.slot, At(h.function));
        h.installed = false;
    }
}

bool Install() {
    gameLogin = reinterpret_cast<LoginFn>(At(LoginFunction));
    gameResult = reinterpret_cast<ResultFn>(At(ResultFunction));
    gameResponse = reinterpret_cast<ResponseFn>(At(ResponseFunction));
    gameSubmit = reinterpret_cast<SubmitFn>(At(SubmitFunction));
    for (auto& h : hooks) {
        h.installed = Write(h.slot, h.replacement);
        if (!h.installed) {
            Uninstall();
            return false;
        }
    }
    bareStatusLine = WriteCode(StatusLineLength, StatusLineBare);
    if (!bareStatusLine)
        Uninstall();
    return bareStatusLine;
}

void Init() {
    gameBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    gameThread = GetCurrentThreadId();
    if (!HostMatches()) {
        MessageBoxW(nullptr,
                    L"" LOGIN_RETRY_NAME " could not start: this version of Spore is not supported. A login that "
                    L"fails is not tried again.\n\n" LOGIN_RETRY_NAME " supports only Spore Galactic Adventures "
                    L"with the March 2017 patch (SporeApp_ModAPIFix.exe).",
                    ModNameWide, MB_OK | MB_ICONERROR);
        return;
    }
    if (!CodeMatches() || !FindRequests() || !Install()) {
        MessageBoxW(nullptr,
                    L"" LOGIN_RETRY_NAME " could not start: the game's login code is not the one it knows, most "
                    L"likely because another mod has changed it. A login that fails is not tried again.",
                    ModNameWide, MB_OK | MB_ICONERROR);
        return;
    }
}

void Dispose() {
    StopTimer();
    attempt.armed = false;
    Uninstall();
}
}

BOOL WINAPI DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        ModAPI::AddPostInitFunction(Init);
        ModAPI::AddDisposeFunction(Dispose);
    }
    return TRUE;
}

