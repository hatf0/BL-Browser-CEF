#ifndef _TORQUE_H_
#define _TORQUE_H_

#include <Windows.h>
#include <memory>
#include "structs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Macros

#define PROJECT "BLBrowser v2"

//Typedef an engine function to use it later
#define BLFUNC(returnType, convention, name, ...)         \
	typedef returnType (convention*name##Fn)(__VA_ARGS__); \
	static name##Fn name;

//Typedef an exported engine function to use it later
#define BLFUNC_EXTERN(returnType, convention, name, ...)  \
	typedef returnType (convention*name##Fn)(__VA_ARGS__); \
	extern name##Fn name;

//Search for an engine function in blockland
#define BLSCAN(target, pattern, mask)            \
	target = (target##Fn)ScanFunc(pattern, mask); \
	if(target == NULL)                             \
		Printf("BaseBlocklandDLL | Cannot find function "#target"!"); 

#define ConsoleMethod(type, function) type ts_##function(SimObject* obj, int argc, const char* argv[])
#define RegisterMethod(name, description, argmin, argmax) AddFunction(NULL, #name, ts_##name, description, argmin, argmax);

//Start of the Blockland.exe module in memory
extern DWORD ImageBase;
extern DWORD ImageSize;

//StringTable pointer
static DWORD StringTable;

void InitScanner(const char* moduleName);
bool SwapVTableEntry(void** vtable, int idx, void* newEntry, void** oldEntry);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Engine function declarations

//Con::printf
BLFUNC_EXTERN(void, , Printf, const char* format, ...);

//Callback types for exposing methods to torquescript
BLFUNC_EXTERN(const char*, __thiscall, CodeBlockExec, CodeBlock* this_, int ip, const char* functionName, Namespace* ns, int argc, const char** argv, bool noCalls, const char* packageName, int setFrame);

BLFUNC_EXTERN(void, , SimInit);

BLFUNC_EXTERN(const char*, __stdcall, StringTableInsert, const char* val, const bool caseSensitive);
//Namespace::find
BLFUNC_EXTERN(Namespace*, __fastcall, NamespaceFind, const char* name, const char* package);
//Namespace::createLocalEntry
BLFUNC_EXTERN(Namespace::Entry*, __thiscall, NamespaceCreateLocalEntry, Namespace * this_, const char* name);
//Namespace::trashCache
BLFUNC_EXTERN(void, , NamespaceTrashCache);

//Dictionary::add
BLFUNC_EXTERN(Dictionary::Entry*, __thiscall, DictionaryAdd, Dictionary * this_, const char* name);
BLFUNC_EXTERN(void, __thiscall, DictionaryAddVariable, Dictionary * this_, const char* name, int type, void* ptr);
BLFUNC_EXTERN(const char*, __thiscall, DictionaryGetVariable, Dictionary * this_, const char* name);
BLFUNC_EXTERN(void, __thiscall, DictionaryEntrySetStringValue, Dictionary::Entry * this_, const char* val);
BLFUNC_EXTERN(const char*, , prependDollar, const char* var);

//Executing code and calling torquescript functions
BLFUNC_EXTERN(CodeBlock*, __thiscall, CodeBlockConstructor, CodeBlock * this_);
BLFUNC_EXTERN(const char*, __thiscall, CodeBlockCompileExec, CodeBlock * this_, const char* fileName, const char* script, bool noCalls);
BLFUNC_EXTERN(void*, __fastcall, dAlloc, size_t size);
BLFUNC_EXTERN(void, __fastcall, dFree, void* buf);
BLFUNC_EXTERN(void, , swapBuffers);

BLFUNC_EXTERN(void, __thiscall, processEvents, void* this_, void* evt);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions

//Scan the module for a pattern
DWORD ScanFunc(const char* pattern, const char* mask);

//Change a byte at a specific location in memory
void PatchByte(BYTE* location, BYTE value);

//Register a torquescript function that returns a string. The function must look like this:
//const char* func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* nameSpace, const char* name, StringCallback callBack, const char* usage, int minArgs, int maxArgs);

//Register a torquescript function that returns an int. The function must look like this:
//int func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* nameSpace, const char* name, IntCallback callBack, const char* usage, int minArgs, int maxArgs);

//Register a torquescript function that returns a float. The function must look like this:
//float func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* nameSpace, const char* name, FloatCallback callBack, const char* usage, int minArgs, int maxArgs);

//Register a torquescript function that returns nothing. The function must look like this:
//void func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* nameSpace, const char* name, VoidCallback callBack, const char* usage, int minArgs, int maxArgs);

//Register a torquescript function that returns a bool. The function must look like this:
//bool func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* nameSpace, const char* name, BoolCallback callBack, const char* usage, int minArgs, int maxArgs);

//Expose an integer variable to torquescript
void AddVariable(const char* name, int* data);

//Expose a boolean variable to torquescript
void AddVariable(const char* name, bool* data);

//Expose a float variable to torquescript
void AddVariable(const char* name, float* data);

//Expose a string variable to torquescript
void AddVariable(const char* name, char* data);

//Get a global variable
const char* GetGlobalVariable(const char* name);
//Set a global variable
void SetGlobalVariable(const char* name, const char* val);

//Evaluate a torquescript string in global scope
const char* Eval(const char* str);

//Initialize the Torque Interface
bool InitTorqueStuff();

#endif