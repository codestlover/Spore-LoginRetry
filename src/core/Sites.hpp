
#pragma once
#include <cstdint>

constexpr unsigned HostImageSize = 26271744;
constexpr unsigned HostTimestamp = 1727737552;
constexpr uint32_t HostPreferredBase = 0x400000;

constexpr uint32_t LoginFunction = 0x609030;
constexpr uint32_t ResultFunction = 0x607840;
constexpr uint32_t ResponseFunction = 0x61de40;
constexpr uint32_t SubmitFunction = 0x6117a0;
constexpr uint32_t RequestsFunction = 0x67caf0;
constexpr uint32_t UserFunction = 0x6e6560;
constexpr uint32_t PasswordFunction = 0x989210;
constexpr uint32_t StatusLineRead = 0x943b5a;
constexpr uint32_t StatusLineCheck = 0x943b74;
constexpr uint32_t StatusLineShape = 0x943bf1;
constexpr uint32_t StatusLineLength = StatusLineCheck + 2;
constexpr unsigned char StatusLineWithPhrase = 0x0e;
constexpr unsigned char StatusLineBare = 0x0d;

constexpr uint32_t ManagerTable = 0x13faf10;
constexpr uint32_t UserSlot = ManagerTable + 0x30;
constexpr uint32_t PasswordSlot = ManagerTable + 0x34;
constexpr uint32_t LoginSlot = ManagerTable + 0x4c;
constexpr uint32_t ResultSlot = ManagerTable + 0x50;

constexpr uint32_t JobTable = 0x13fc8b4;
constexpr uint32_t ResponseSlot = JobTable + 0x18;

constexpr unsigned UserOffset = 0x4;
constexpr unsigned PasswordOffset = 0x14;
constexpr unsigned StatusOffset = 0x43c;

struct Signature {
    uint32_t address;
    unsigned size;
    unsigned char bytes[20];
};

constexpr Signature kSignatures[] = {
    {LoginFunction, 10, {0x53, 0x55, 0x56, 0x8b, 0xe9, 0xe8, 0xe6, 0x81, 0x34, 0x00}},
    {ResultFunction,
     17,
     {0x83, 0xec, 0x14, 0x56, 0x57, 0x66, 0x8b, 0x7c, 0x24, 0x20, 0xb8, 0x07, 0x00, 0x00, 0x00, 0x8b, 0xf1}},
    {ResponseFunction, 11, {0x8b, 0x44, 0x24, 0x08, 0x53, 0x56, 0x57, 0x8b, 0xf1, 0x85, 0xc0}},
    {SubmitFunction, 20, {0x53, 0x56, 0x8b, 0xd9, 0x57, 0x8d, 0x7b, 0x08, 0x8b, 0xcf,
                          0xe8, 0x01, 0xfa, 0xff, 0xff, 0xe8, 0x3c, 0xb3, 0x06, 0x00}},
    {RequestsFunction, 1, {0xa1}},
    {RequestsFunction + 5, 1, {0xc3}},
    {UserFunction, 4, {0x8d, 0x41, 0x04, 0xc3}},
    {PasswordFunction, 4, {0x8d, 0x41, 0x14, 0xc3}},
    {StatusLineRead, 12, {0x8b, 0x8e, 0x44, 0x01, 0x00, 0x00, 0x8b, 0x96, 0x40, 0x01, 0x00, 0x00}},
    {StatusLineCheck, 5, {0x83, 0xfd, StatusLineWithPhrase, 0x7d, 0x17}},
    {StatusLineShape, 20, {0xb1, 0x20, 0x38, 0x48, 0x08, 0x0f, 0x85, 0xbe, 0x00, 0x00,
                           0x00, 0x38, 0x48, 0x0c, 0x0f, 0x85, 0xb5, 0x00, 0x00, 0x00}},
};

struct Slot {
    uint32_t address;
    uint32_t function;
};

constexpr Slot kSlots[] = {
    {UserSlot, UserFunction},     {PasswordSlot, PasswordFunction}, {LoginSlot, LoginFunction},
    {ResultSlot, ResultFunction}, {ResponseSlot, ResponseFunction},
};

