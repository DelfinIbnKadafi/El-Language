#ifndef ELPLUGIN_H
#define ELPLUGIN_H

// EL plugin API, version 1.
//
// A plugin is a shared library (.dll on Windows, .so on Linux, .dylib on
// macOS) that adds native functions to EL. This is the only file a plugin
// needs: it does not link against elvm, everything goes through the ELApi
// table that elvm hands to elp_Load().
//
// A plugin exports these C functions:
//
//   int  elp_Supports(void);            required, return EL_PLUGIN_API_VERSION
//   int  elp_Load(const ELApi* api);    required, register natives, 0 = ok
//   void elp_Unload(void);              optional, called when elvm exits
//
// Any language that can export C functions can be used (C, C++, Rust, Go,
// Zig, ...).

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define EL_EXPORT __declspec(dllexport)
#else
#define EL_EXPORT __attribute__((visibility("default")))
#endif

#define EL_PLUGIN_API_VERSION 1

// One running call of a native function. Only valid inside that call.
typedef struct ELCall ELCall;

// A native function. Read the arguments with the Get* functions and give the
// result with one Return* function.
typedef void (*ELNative)(ELCall* call);

// Table of functions elvm gives to the plugin. 'size' is the size of the
// table as elvm knows it, so a newer plugin can check a field exists before
// using it. New fields are only ever added at the end.
typedef struct ELApi {
  unsigned int size;
  unsigned int version;
  
  // Register a native function. 'signature' is written like this:
  //   "name(type, type, ...):returntype"
  // where each type is int, float, bool or str, and ":returntype" is left out
  // for a function that returns nothing. Examples:
  //   "hash_djb2(str):int"
  //   "mysql_connect(str, str, str, str, int):int"
  //   "log_write(str)"
  // Returns 1 on success, 0 if the signature is not valid.
  int (*Register)(const char* signature, ELNative fn);
  
  // Number of arguments of this call (always the registered count)
  int (*ArgCount)(ELCall* call);
  
  // 1 if argument 'index' (counted from 0) is NONE
  int (*IsNone)(ELCall* call, int index);
  
  // Read argument 'index'. A NONE argument reads as 0, 0.0, 0 or "".
  // The pointer from GetStr is only valid until the native returns, copy it
  // if the plugin needs to keep it.
  long long (*GetInt)(ELCall* call, int index);
  double (*GetFloat)(ELCall* call, int index);
  int (*GetBool)(ELCall* call, int index);
  const char* (*GetStr)(ELCall* call, int index);
  
  // Give the result of the call. ReturnStr copies the text, so the plugin
  // keeps ownership of its own memory. An EL int is a 32 bit number: if a
  // function declared ":int" gives a value outside -2147483648..2147483647,
  // the program stops with a runtime error instead of using a wrong number.
  // Return such a value as a float or a str instead.
  void (*ReturnInt)(ELCall* call, long long value);
  void (*ReturnFloat)(ELCall* call, double value);
  void (*ReturnBool)(ELCall* call, int value);
  void (*ReturnStr)(ELCall* call, const char* value);
  void (*ReturnNone)(ELCall* call);
  
  // Stop the program with a runtime error "file (line) : message". The native
  // should return right after calling it.
  void (*Error)(ELCall* call, const char* format, ...);
} ELApi;

#ifdef __cplusplus
}
#endif

#endif
