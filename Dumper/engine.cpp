#include <Windows.h>
#include "engine.h"
#include "generic.h"
#include "memory.h"
#include "utils.h"
#include "wrappers.h"

Offsets offsets;

ansi_fn Decrypt_ANSI = nullptr;
// wide_fn Decrypt_WIDE = nullptr;


struct {
  uint16 Stride = 4;
  struct {
    uint16 Size = 32; 
  } FUObjectItem;
  struct {
    uint16 Number = 12;
  } FName;
  struct {
      uint16 Info = 4;
      uint16 WideBit = 0;
      uint16 LenBit = 6;
      uint16 HeaderSize = 6;
  } FNameEntry;
  struct
  {
      uint16 Index = 0x10;
      uint16 Class = 0x18;
      uint16 Name = 0x20;
      uint16 Outer = 0x28;
  } UObject;
  struct {
    uint16 Next = 0x38;
  } UField;
  struct {
    uint16 SuperStruct = 0x48;
    uint16 Children = 0x58;
    uint16 ChildProperties = 0x60;
    uint16 PropertiesSize = 0x68;
  } UStruct;
  struct {
      uint16 Names = 0x50;
  } UEnum;
  struct {
    uint16 FunctionFlags = 0xC0;
    uint16 Func = 0xC0 + 0x28; // ue3-ue4, always +0x28 from flags location.
  } UFunction;
  struct {
      uint16 Class = 0x8;
      uint16 Next = 0x20;
      uint16 Name = 0x28;
  } FField;
  struct {
      uint16 ArrayDim = 0x38;
      uint16 ElementSize = 0x3C;
      uint16 PropertyFlags = 0x40;
      uint16 Offset = 0x4C;
      uint16 Size = 0x78;
  } FProperty;
  struct {
      uint16 ArrayDim = 0;
      uint16 ElementSize = 0;
      uint16 PropertyFlags = 0;
      uint16 Offset = 0;
      uint16 Size = 0; // sizeof(UProperty)
  } UProperty;
} DeadByDaylight;
static_assert(sizeof(DeadByDaylight) == sizeof(Offsets));


struct {
  void* offsets; // address to filled offsets structure
  std::pair<const char*, uint32> names; // NamePoolData signature
  std::pair<const char*, uint32> objects; // ObjObjects signature
  std::function<bool(void*, void*)> callback;
} engines[] = {
  { // RogueCompany | PropWitchHuntModule-Win64-Shipping | Scum
    &DeadByDaylight,
    {"\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x01\x0F\x10\x03\x4C\x8D\x44\x24\x20\x48\x8B\xC8", 30},
    {"\x48\x8B\x00\x00\x00\x00\x00\x48\x63\x00\x48\xC1\xE2\x00\x48\x03\x00\x00\x74\x00\x44\x39\x00\x00\x75\x00\xF7\x42\x10\x00\x00\x00\x00\x75\x00\xB0", 36},
    nullptr
  },
};

std::unordered_map<std::string, decltype(&engines[0])> games = {
  {"Prospect-Win64-Shipping",&engines[0]},
};

STATUS EngineInit(std::string game, void* image) {

  auto it = games.find(game);
  if (it == games.end()) { return STATUS::ENGINE_NOT_FOUND; }

  auto engine = it->second;
  offsets = *(Offsets*)(engine->offsets);

  void* names = nullptr; 
  void* objects = nullptr;

  uint8 found = 0;
  if (!engine->callback) {
    found |= 4;
  }

  IterateExSections(
    image,
    [&](void* start, void* end)->bool {
      if (!(found & 1)) if (names = FindPointer(start, end, engine->names.first, engine->names.second)) found |= 1;
      if (!(found & 2)) if (objects = FindPointer(start, end, engine->objects.first, engine->objects.second)) found |= 2;
      if (!(found & 4)) if (engine->callback(start, end)) found |= 4;
      if (found == 7) return 1;
      return 0;
    }
  );

  if (found != 7) return STATUS::ENGINE_FAILED;

  NamePoolData = *(decltype(NamePoolData)*)names;
  ObjObjects = Read<TUObjectArray>((void*)(0x7FF63E7B84F0));

  auto entry = UE_FNameEntry(NamePoolData.GetEntry(0));

  // exception handler exclusively for Decrypt_ANSI
  try {
    if (*(uint32*)entry.String().data() != 'enoN') return STATUS::ENGINE_FAILED;
  }
  catch (...) {
    return STATUS::ENGINE_FAILED;
  }

  return STATUS::SUCCESS;
}
