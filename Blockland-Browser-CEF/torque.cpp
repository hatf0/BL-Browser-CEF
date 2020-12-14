#include "Torque.h"
#include "structs.h"
#include <stdio.h>
#include <Psapi.h>

// Con::printf
PrintfFn Printf;
// CodeBlock::Exec
CodeBlockExecFn CodeBlockExec;
// Sim::Init (unnecessary - we're loaded early enough)
SimInitFn SimInit;
// _StringTable::Insert
StringTableInsertFn StringTableInsert;
// Namespace::find
NamespaceFindFn NamespaceFind;
// Namespace::CreateLocalEntry
NamespaceCreateLocalEntryFn NamespaceCreateLocalEntry;
// Namespace::trashCache
NamespaceTrashCacheFn NamespaceTrashCache;
// Dictionary::add
DictionaryAddFn DictionaryAdd;
// Dictionary::addVariable
DictionaryAddVariableFn DictionaryAddVariable;
// Dictionary::getVariable
DictionaryGetVariableFn DictionaryGetVariable;
// Dictionary::Entry::SetStringValue
DictionaryEntrySetStringValueFn DictionaryEntrySetStringValue;
// prependDollar ties into getVariable
prependDollarFn prependDollar;
// CodeBlock::CodeBlock
CodeBlockConstructorFn CodeBlockConstructor;
// CodeBlock::compileExec
CodeBlockCompileExecFn CodeBlockCompileExec;
// dork allocators
dAllocFn dAlloc;
dFreeFn dFree;
swapBuffersFn swapBuffers;

DWORD ImageBase = NULL;
DWORD ImageSize = NULL;

// override the default allocators to use dork allocators
// void* operator new (size_t size) { return dAlloc(size); }
// void* operator new[] (size_t size) { return dAlloc(size); }

// void operator delete (void* buf) { dFree(buf); }
// void operator delete[](void* buf) { dFree(buf); }

//Set the module start and length
void InitScanner(const char* moduleName)
{
	HMODULE module = GetModuleHandleA(moduleName);
	if (!module) return;

	MODULEINFO info;
	GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

	ImageBase = (DWORD)info.lpBaseOfDll;
	ImageSize = info.SizeOfImage;
}

bool CompareData(PBYTE data, PBYTE pattern, char* mask)
{
	//Iterate over the data, pattern and mask in parallel
	for (; *mask; ++data, ++pattern, ++mask)
	{
		//And check for equality at each unmasked byte
		if (*mask == 'x' && *data != *pattern)
			return false;
	}

	return (*mask) == NULL;
}

//Find a pattern in memory
DWORD FindPattern(DWORD imageBase, DWORD imageSize, PBYTE pattern, char* mask)
{
	for (DWORD i = imageBase; i < imageBase + imageSize; i++)
	{
		//check for matching pattern at every byte
		if (CompareData((PBYTE)i, pattern, mask))
			return i;
	}

	return 0;
}

bool SwapVTableEntry(void** vtable, int idx, void* newEntry, void** oldEntry) {
	Printf("%s - Hooking vtable at address %x (idx: %d)", PROJECT, vtable, idx);
	Printf("%s - Pre-hook address: %x", PROJECT, vtable[idx]);
	DWORD oldProtection;
	DWORD protectSize = (idx * sizeof(void*) + 4); // set page to RW (and add a bit of margin just in case)
	{
		VirtualProtect(vtable, protectSize, PAGE_EXECUTE_READWRITE, &oldProtection);
		if (oldEntry) {
			*oldEntry = vtable[idx];
		}
		vtable[idx] = newEntry;
		VirtualProtect(vtable, protectSize, oldProtection, &oldProtection);
	}

	if (!oldEntry) {
		Printf("%s - Restoring vtable entry worked");
		Printf("%s - New vtable entry: %x", PROJECT, vtable[10]);
		return true;
	}
	else {
		if (*oldEntry != vtable[idx]) {
			Printf("%s - Hooking worked", PROJECT);
			Printf("%s - Old vtable entry: %x", PROJECT, *oldEntry);
			Printf("%s - New vtable entry: %x", PROJECT, vtable[10]);
			return true;
		}
		else {
			Printf("%s - Hooking failed, game may crash", PROJECT);
			return false;
		}
	}
}

//Scan the module for a pattern
DWORD ScanFunc(const char* pattern, const char* mask)
{
	return FindPattern(ImageBase, ImageSize - strlen(mask), (PBYTE)pattern, (char*)mask);
}


// this is our shim function that basically lets us hook in 
// it replicates the function (which is inlined)
Namespace::Entry* InsertFunction(const char* nameSpace, const char* name) {
	Namespace* ns = NULL;
	if (ns) {
		ns = NamespaceFind(StringTableInsert(nameSpace, 0), 0);
	}
	else {
		ns = mGlobalNamespace;
	}

	Namespace::Entry* entry = NamespaceCreateLocalEntry(ns, StringTableInsert(name, 0));
	NamespaceTrashCache();
	return entry;
}

//Register a torquescript function that returns a string. The function must look like this:
//const char* func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* ns, const char* name, StringCallback cb, const char* usage, int minArgs, int maxArgs)
{
	Namespace::Entry* func = InsertFunction(ns, name);
	func->mUsage = usage;
	func->mMaxArgs = maxArgs;
	func->mMinArgs = minArgs;
	func->mType = Namespace::Entry::StringCallbackType;
	func->cb.mStringCallbackFunc = cb;
}

//Register a torquescript function that returns an int. The function must look like this:
//int func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* ns, const char* name, IntCallback cb, const char* usage, int minArgs, int maxArgs)
{
	Namespace::Entry* func = InsertFunction(ns, name);
	func->mUsage = usage;
	func->mMaxArgs = maxArgs;
	func->mMinArgs = minArgs;
	func->mType = Namespace::Entry::IntCallbackType;
	func->cb.mIntCallbackFunc = cb;
}

//Register a torquescript function that returns a float. The function must look like this:
//float func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* ns, const char* name, FloatCallback cb, const char* usage, int minArgs, int maxArgs)
{
	Namespace::Entry* func = InsertFunction(ns, name);
	func->mUsage = usage;
	func->mMaxArgs = maxArgs;
	func->mMinArgs = minArgs;
	func->mType = Namespace::Entry::FloatCallbackType;
	func->cb.mFloatCallbackFunc = cb;
}

//Register a torquescript function that returns nothing. The function must look like this:
//void func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* ns, const char* name, VoidCallback cb, const char* usage, int minArgs, int maxArgs)
{
	Namespace::Entry* func = InsertFunction(ns, name);
	func->mUsage = usage;
	func->mMaxArgs = maxArgs;
	func->mMinArgs = minArgs;
	func->mType = Namespace::Entry::VoidCallbackType;
	func->cb.mVoidCallbackFunc = cb;
}

//Register a torquescript function that returns a bool. The function must look like this:
//bool func(DWORD* obj, int argc, const char* argv[])
void AddFunction(const char* ns, const char* name, BoolCallback cb, const char* usage, int minArgs, int maxArgs)
{
	Namespace::Entry* func = InsertFunction(ns, name);
	func->mUsage = usage;
	func->mMaxArgs = maxArgs;
	func->mMinArgs = minArgs;
	func->mType = Namespace::Entry::BoolCallbackType;
	func->cb.mBoolCallbackFunc = cb;
}

//Expose an integer variable to torquescript
void AddVariable(const char* name, int* data)
{
	DictionaryAddVariable(&gEvalState->globalVars, StringTableInsert(name, 0), 4, data);
}

//Expose a boolean variable to torquescript
void AddVariable(const char* name, bool* data)
{
	DictionaryAddVariable(&gEvalState->globalVars, StringTableInsert(name, 0), 6, data);
}

//Expose a float variable to torquescript
void AddVariable(const char* name, float* data)
{
	DictionaryAddVariable(&gEvalState->globalVars, StringTableInsert(name, 0), 8, data);
}

//Expose a string variable to torquescript
void AddVariable(const char* name, char* data)
{
	DictionaryAddVariable(&gEvalState->globalVars, StringTableInsert(name, 0), 10, data);
}

//Get a global variable
const char* GetGlobalVariable(const char* name) {
	return DictionaryGetVariable(&gEvalState->globalVars, StringTableInsert(name, 0));
}

//Set a global variable
void SetGlobalVariable(const char* name, const char* val) {
	Dictionary::Entry* entry = DictionaryAdd(&gEvalState->globalVars, StringTableInsert(prependDollar(name), 0));
	DictionaryEntrySetStringValue(entry, val);
}

//Evaluate a torquescript string in global scope
const char* Eval(const char* str)
{
	CodeBlock* block = new CodeBlock();
	block = CodeBlockConstructor(block);
	return CodeBlockCompileExec(block, NULL, str, false);
}

//Initialize the Torque Interface
bool InitTorqueStuff()
{
	InitScanner("Blockland.exe");

	dFree = (dFreeFn)(ImageBase + 0x176E40);
	dAlloc = (dAllocFn)(ImageBase + 0x178160);
	if (!dFree || !dAlloc) // we need the allocators, die if we don't have them
		return false;

	Printf = (PrintfFn)(ImageBase + 0x37BC0);
	if (!Printf) // we need printf for debug output
		return false;

	StringTableInsert = (StringTableInsertFn)(ImageBase + 0x053910);
	NamespaceFind = (NamespaceFindFn)(ImageBase + 0x41060);
	mGlobalNamespace = NamespaceFind(NULL, NULL);
	gEvalState = (ExprEvalState*)(ImageBase + 0x384800);
	NamespaceCreateLocalEntry = (NamespaceCreateLocalEntryFn)(ImageBase + 0x0415A0);
	NamespaceTrashCache = (NamespaceTrashCacheFn)(ImageBase + 0x3D7C0);
	DictionaryAdd = (DictionaryAddFn)(ImageBase + 0x40720);
	DictionaryAddVariable = (DictionaryAddVariableFn)(ImageBase + 0x40D20);
	DictionaryGetVariable = (DictionaryGetVariableFn)(ImageBase + 0x40BC0);
	DictionaryEntrySetStringValue = (DictionaryEntrySetStringValueFn)(ImageBase + 0x40C10);
	prependDollar = (prependDollarFn)(ImageBase + 0x36ED0);
	CodeBlockConstructor = (CodeBlockConstructorFn)(ImageBase + 0x335B0);
	CodeBlockCompileExec = (CodeBlockCompileExecFn)(ImageBase + 0x340A0);
	swapBuffers = (swapBuffersFn)ScanFunc(
		"\x55\x8B\xEC\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x81\xEC\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x33\xC5\x89\x45\xF0\x53\x56\x57\x50\x8D\x45\xF4\x64\xA3\x00\x00\x00\x00\x80\x3D",
		"xxxxxx????xx????xxx????x????xxxxxxxxxxxxxx????xx");

	return true;
}