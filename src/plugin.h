#ifndef PLUGIN_H
#define PLUGIN_H

#include "elvm.h"
#include "elplugin.h"

// Plugin loader and native function registry. A plugin is a shared library
// listed in the "plugins" field of library.json, see elplugin.h for the API
// a plugin is written against.

// One native function registered by a plugin
typedef struct {
  char* name;
  
  int paramCount;
  VarType* paramTypes;
  
  // 0 for a function that returns nothing
  int hasReturn;
  VarType returnType;
  
  ELNative function;
  
  // Index of the plugin that registered it, see PluginDisplayName
  int pluginIndex;
} PluginNative;

// Result of loading a plugin
#define PLUGIN_LOADED 0
#define PLUGIN_ALREADY_LOADED 1
#define PLUGIN_FAILED 2

// Extension plugins use on this system (".dll", ".so" or ".dylib")
const char* PluginPlatformExtension(void);

// Decide whether a "plugins" entry of library.json is for this system. An
// entry with a known extension (.dll, .so, .dylib) is only for the system that
// uses that extension. An entry with no known extension gets this system's
// extension added. Returns 1 and a new heap string in 'resolved' if the entry
// is for this system, or 0 if it is for another one.
int PluginResolveFileName(const char* entry, char** resolved);

// Load the plugin at 'path' and run its elp_Load. 'displayName' is used in
// messages. On PLUGIN_LOADED, the natives it registered are the ones from
// *firstNative up to PluginNativeCount(). On PLUGIN_FAILED a message is copied
// into 'error'.
int PluginLoad(const char* path, const char* displayName, int* firstNative, char* error, int errorSize);

int PluginNativeCount(void);
const PluginNative* PluginNativeAt(int index);

// Name of the plugin a native came from, for error messages
const char* PluginDisplayName(int pluginIndex);

// Result of running a native
typedef struct {
  int isNone;
  double number;
  
  // Heap string for a str result, freed by the caller. NULL otherwise.
  char* string;
} PluginResult;

// Run native 'index' with its already evaluated arguments. Returns 1 if it
// worked, or 0 with a message in 'error' if the plugin reported an error.
int PluginCallNative(int index, const VarType* types, const double* numbers,
  const int* isNone, char* const* strings, PluginResult* result, char* error, int errorSize);

#endif
