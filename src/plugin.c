#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include "plugin.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define PLUGIN_EXTENSION ".dll"
#elif defined(__APPLE__)
#define PLUGIN_EXTENSION ".dylib"
#else
#define PLUGIN_EXTENSION ".so"
#endif

// Size of the buffer for error text coming from a plugin
#define PLUGIN_ERROR_SIZE 512

typedef int (*SupportsFunction)(void);
typedef int (*LoadFunction)(const ELApi* api);
typedef void (*UnloadFunction)(void);

// One loaded plugin
typedef struct {
  char* path;
  char* displayName;
  void* handle;
  UnloadFunction unload;
} LoadedPlugin;

static LoadedPlugin* plugins = NULL;
static int pluginCount = 0;
static int pluginCapacity = 0;

static PluginNative* natives = NULL;
static int nativeCount = 0;
static int nativeCapacity = 0;

// Plugin whose elp_Load is running right now, and whether one of its
// Register calls got a signature that is not valid
static int loadingPlugin = -1;
static int registerFailed = 0;
static char registerError[PLUGIN_ERROR_SIZE];

static int unloadRegistered = 0;

// One running call of a native function
struct ELCall {
  const PluginNative* native;
  const VarType* types;
  const double* numbers;
  const int* isNone;
  char* const* strings;
  
  int returned;
  int retIsNone;
  double retNumber;
  char* retString;
  
  int failed;
  char error[PLUGIN_ERROR_SIZE];
};

const char* PluginPlatformExtension(void) {
  return PLUGIN_EXTENSION;
}

// Length of the extension at the end of 'name' if it is one of the known
// plugin extensions, or 0. 'isThisSystem' tells if it is this system's own.
static int KnownExtension(const char* name, int* isThisSystem) {
  static const char* extensions[] = {".dll", ".so", ".dylib"};
  size_t length = strlen(name);
  
  for(int i = 0; i < 3; i++) {
    size_t extensionLength = strlen(extensions[i]);
    
    if(length > extensionLength) {
      const char* tail = name + length - extensionLength;
      int same = 1;
      
      for(size_t j = 0; j < extensionLength; j++) {
        if(tolower((unsigned char) tail[j]) != extensions[i][j]) {
          same = 0;
          break;
        }
      }
      
      if(same) {
        *isThisSystem = strcmp(extensions[i], PLUGIN_EXTENSION) == 0;
        return (int) extensionLength;
      }
    }
  }
  
  return 0;
}

int PluginResolveFileName(const char* entry, char** resolved) {
  int isThisSystem = 0;
  int extensionLength = KnownExtension(entry, &isThisSystem);
  
  if(extensionLength > 0 && !isThisSystem) {
    return 0;
  }
  
  size_t length = strlen(entry);
  
  if(extensionLength > 0) {
    *resolved = malloc(length + 1);
    strcpy(*resolved, entry);
  } else {
    *resolved = malloc(length + strlen(PLUGIN_EXTENSION) + 1);
    strcpy(*resolved, entry);
    strcat(*resolved, PLUGIN_EXTENSION);
  }
  
  return 1;
}

// Text of the last error of the dynamic loader
static void LoaderError(char* error, int errorSize) {
#ifdef _WIN32
  DWORD code = GetLastError();
  char text[256] = "";
  
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, text, sizeof(text), NULL);
  
  for(char* end = text + strlen(text); end > text && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '); end--) {
    end[-1] = '\0';
  }
  
  snprintf(error, errorSize, "error %lu%s%s", (unsigned long) code, text[0] != '\0' ? ": " : "", text);
#else
  const char* text = dlerror();
  
  snprintf(error, errorSize, "%s", text != NULL ? text : "unknown error");
#endif
}

static void* OpenLibrary(const char* path) {
#ifdef _WIN32
  // LoadLibrary wants '\' and, with this flag, also looks for the DLLs the
  // plugin itself needs in the plugin's own folder
  char* windowsPath = malloc(strlen(path) + 1);
  
  strcpy(windowsPath, path);
  
  for(char* c = windowsPath; *c != '\0'; c++) {
    if(*c == '/') {
      *c = '\\';
    }
  }
  
  void* handle = (void*) LoadLibraryExA(windowsPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  
  free(windowsPath);
  return handle;
#else
  return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

// Find an exported function. A function pointer is not converted from a data
// pointer directly, to stay valid C.
static int FindSymbol(void* handle, const char* name, void* function, size_t functionSize) {
  void* symbol;
  
#ifdef _WIN32
  symbol = (void*) GetProcAddress((HMODULE) handle, name);
#else
  symbol = dlsym(handle, name);
#endif
  
  if(symbol == NULL) {
    return 0;
  }
  
  memcpy(function, &symbol, functionSize);
  return 1;
}

static void CloseLibrary(void* handle) {
#ifdef _WIN32
  FreeLibrary((HMODULE) handle);
#else
  dlclose(handle);
#endif
}

// Call elp_Unload of every plugin, last loaded first. Runs when elvm exits,
// in every way it can exit (end of the program, 'exit', or a runtime error).
static void UnloadAllPlugins(void) {
  for(int i = pluginCount - 1; i >= 0; i--) {
    if(plugins[i].unload != NULL) {
      plugins[i].unload();
      plugins[i].unload = NULL;
    }
  }
}

// Read a type name (int, float, bool, str) from a signature
static int ParseSignatureType(const char** cursor, VarType* type) {
  const char* p = *cursor;
  
  while(*p == ' ' || *p == '\t') {
    p++;
  }
  
  static const struct { const char* word; VarType type; } words[] = {
    {"int", VAR_INT}, {"float", VAR_FLOAT}, {"bool", VAR_BOOL}, {"str", VAR_STR}
  };
  
  for(int i = 0; i < 4; i++) {
    size_t length = strlen(words[i].word);
    
    if(strncmp(p, words[i].word, length) == 0 && !isalnum((unsigned char) p[length]) && p[length] != '_') {
      *type = words[i].type;
      *cursor = p + length;
      return 1;
    }
  }
  
  return 0;
}

static void SkipBlanks(const char** cursor) {
  while(**cursor == ' ' || **cursor == '\t') {
    (*cursor)++;
  }
}

static int RegisterNative(const char* signature, ELNative function) {
  if(loadingPlugin < 0) {
    return 0;
  }
  
  PluginNative native;
  
  memset(&native, 0, sizeof(native));
  
  const char* p = signature;
  
  if(signature == NULL || function == NULL) {
    snprintf(registerError, sizeof(registerError), "Register needs a signature and a function");
    registerFailed = 1;
    return 0;
  }
  
  SkipBlanks(&p);
  
  const char* nameStart = p;
  
  if(isalpha((unsigned char) *p) || *p == '_') {
    while(isalnum((unsigned char) *p) || *p == '_') {
      p++;
    }
  }
  
  size_t nameLength = (size_t) (p - nameStart);
  
  if(nameLength == 0 || nameLength >= TOKEN_MAX_LEN) {
    snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (bad function name)", signature);
    registerFailed = 1;
    return 0;
  }
  
  SkipBlanks(&p);
  
  if(*p != '(') {
    snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (expected '(')", signature);
    registerFailed = 1;
    return 0;
  }
  
  p++;
  SkipBlanks(&p);
  
  int capacity = 0;
  
  if(*p != ')') {
    while(1) {
      VarType type;
      
      if(!ParseSignatureType(&p, &type)) {
        snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (expected int, float, bool or str)", signature);
        registerFailed = 1;
        free(native.paramTypes);
        return 0;
      }
      
      if(native.paramCount >= capacity) {
        capacity = capacity == 0 ? 4 : capacity * 2;
        native.paramTypes = realloc(native.paramTypes, capacity * sizeof(VarType));
      }
      
      native.paramTypes[native.paramCount++] = type;
      
      SkipBlanks(&p);
      
      if(*p == ',') {
        p++;
        continue;
      }
      
      break;
    }
  }
  
  if(*p != ')') {
    snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (expected ')')", signature);
    registerFailed = 1;
    free(native.paramTypes);
    return 0;
  }
  
  p++;
  SkipBlanks(&p);
  
  if(*p == ':') {
    p++;
    
    if(!ParseSignatureType(&p, &native.returnType)) {
      snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (bad return type)", signature);
      registerFailed = 1;
      free(native.paramTypes);
      return 0;
    }
    
    native.hasReturn = 1;
    SkipBlanks(&p);
  }
  
  if(*p != '\0') {
    snprintf(registerError, sizeof(registerError), "Invalid native signature \"%s\" (unexpected text at the end)", signature);
    registerFailed = 1;
    free(native.paramTypes);
    return 0;
  }
  
  native.name = malloc(nameLength + 1);
  memcpy(native.name, nameStart, nameLength);
  native.name[nameLength] = '\0';
  
  native.function = function;
  native.pluginIndex = loadingPlugin;
  
  if(nativeCount >= nativeCapacity) {
    nativeCapacity = nativeCapacity == 0 ? 16 : nativeCapacity * 2;
    natives = realloc(natives, nativeCapacity * sizeof(PluginNative));
  }
  
  natives[nativeCount++] = native;
  return 1;
}

// Failing call: keep the first error only
static void CallFail(ELCall* call, const char* format, ...) {
  if(call->failed) {
    return;
  }
  
  va_list args;
  
  va_start(args, format);
  vsnprintf(call->error, sizeof(call->error), format, args);
  va_end(args);
  
  call->failed = 1;
}

static int ArgCount(ELCall* call) {
  return call->native->paramCount;
}

// True if 'index' is an argument of this call
static int CheckIndex(ELCall* call, int index) {
  if(index < 0 || index >= call->native->paramCount) {
    CallFail(call, "Plugin function '%s': argument %d does not exist", call->native->name, index);
    return 0;
  }
  
  return 1;
}

static int IsNone(ELCall* call, int index) {
  return CheckIndex(call, index) ? call->isNone[index] : 0;
}

// True if argument 'index' is a number (int, float or bool)
static int CheckNumberArg(ELCall* call, int index) {
  if(!CheckIndex(call, index)) {
    return 0;
  }
  
  if(call->types[index] == VAR_STR) {
    CallFail(call, "Plugin function '%s': argument %d is a str, not a number", call->native->name, index);
    return 0;
  }
  
  return 1;
}

static long long GetInt(ELCall* call, int index) {
  if(!CheckNumberArg(call, index) || call->isNone[index]) {
    return 0;
  }
  
  return (long long) call->numbers[index];
}

static double GetFloat(ELCall* call, int index) {
  if(!CheckNumberArg(call, index) || call->isNone[index]) {
    return 0;
  }
  
  return call->numbers[index];
}

static int GetBool(ELCall* call, int index) {
  if(!CheckNumberArg(call, index) || call->isNone[index]) {
    return 0;
  }
  
  return call->numbers[index] != 0;
}

static const char* GetStr(ELCall* call, int index) {
  if(!CheckIndex(call, index)) {
    return "";
  }
  
  if(call->types[index] != VAR_STR) {
    CallFail(call, "Plugin function '%s': argument %d is not a str", call->native->name, index);
    return "";
  }
  
  if(call->isNone[index] || call->strings[index] == NULL) {
    return "";
  }
  
  return call->strings[index];
}

// True if the native can give a number as its result
static int CheckNumberReturn(ELCall* call) {
  if(!call->native->hasReturn) {
    CallFail(call, "Plugin function '%s' returns nothing, but the plugin gave a result", call->native->name);
    return 0;
  }
  
  if(call->native->returnType == VAR_STR) {
    CallFail(call, "Plugin function '%s' returns a str, but the plugin gave a number", call->native->name);
    return 0;
  }
  
  return 1;
}

static void SetNumberResult(ELCall* call, double value) {
  if(!CheckNumberReturn(call)) {
    return;
  }
  
  call->returned = 1;
  call->retIsNone = 0;
  call->retNumber = value;
}

static void ReturnInt(ELCall* call, long long value) {
  SetNumberResult(call, (double) value);
}

static void ReturnFloat(ELCall* call, double value) {
  SetNumberResult(call, value);
}

static void ReturnBool(ELCall* call, int value) {
  SetNumberResult(call, value != 0 ? 1 : 0);
}

static void ReturnStr(ELCall* call, const char* value) {
  if(!call->native->hasReturn) {
    CallFail(call, "Plugin function '%s' returns nothing, but the plugin gave a result", call->native->name);
    return;
  }
  
  if(call->native->returnType != VAR_STR) {
    CallFail(call, "Plugin function '%s' returns a number, but the plugin gave a str", call->native->name);
    return;
  }
  
  const char* text = value != NULL ? value : "";
  
  free(call->retString);
  
  call->retString = malloc(strlen(text) + 1);
  strcpy(call->retString, text);
  
  call->returned = 1;
  call->retIsNone = 0;
}

static void ReturnNone(ELCall* call) {
  if(!call->native->hasReturn) {
    CallFail(call, "Plugin function '%s' returns nothing, but the plugin gave a result", call->native->name);
    return;
  }
  
  call->returned = 1;
  call->retIsNone = 1;
}

static void ErrorFromPlugin(ELCall* call, const char* format, ...) {
  if(call->failed) {
    return;
  }
  
  va_list args;
  
  va_start(args, format);
  vsnprintf(call->error, sizeof(call->error), format, args);
  va_end(args);
  
  call->failed = 1;
}

static const ELApi pluginApi = {
  sizeof(ELApi),
  EL_PLUGIN_API_VERSION,
  RegisterNative,
  ArgCount,
  IsNone,
  GetInt,
  GetFloat,
  GetBool,
  GetStr,
  ReturnInt,
  ReturnFloat,
  ReturnBool,
  ReturnStr,
  ReturnNone,
  ErrorFromPlugin
};

int PluginLoad(const char* path, const char* displayName, int* firstNative, char* error, int errorSize) {
  for(int i = 0; i < pluginCount; i++) {
    if(strcmp(plugins[i].path, path) == 0) {
      return PLUGIN_ALREADY_LOADED;
    }
  }
  
  void* handle = OpenLibrary(path);
  
  if(handle == NULL) {
    char reason[256];
    
    LoaderError(reason, sizeof(reason));
    snprintf(error, errorSize, "Cannot load plugin \"%s\" (%s)", displayName, reason);
    return PLUGIN_FAILED;
  }
  
  SupportsFunction supports = NULL;
  LoadFunction load = NULL;
  UnloadFunction unload = NULL;
  
  if(!FindSymbol(handle, "elp_Supports", &supports, sizeof(supports)) ||
     !FindSymbol(handle, "elp_Load", &load, sizeof(load))) {
    snprintf(error, errorSize, "\"%s\" is not an EL plugin (it must export elp_Supports and elp_Load)", displayName);
    CloseLibrary(handle);
    return PLUGIN_FAILED;
  }
  
  FindSymbol(handle, "elp_Unload", &unload, sizeof(unload));
  
  int version = supports();
  
  if(version != EL_PLUGIN_API_VERSION) {
    snprintf(error, errorSize, "Plugin \"%s\" is made for plugin API version %d, this ELVM uses version %d",
      displayName, version, EL_PLUGIN_API_VERSION);
    CloseLibrary(handle);
    return PLUGIN_FAILED;
  }
  
  if(pluginCount >= pluginCapacity) {
    pluginCapacity = pluginCapacity == 0 ? 4 : pluginCapacity * 2;
    plugins = realloc(plugins, pluginCapacity * sizeof(LoadedPlugin));
  }
  
  LoadedPlugin* plugin = &plugins[pluginCount];
  
  plugin->path = malloc(strlen(path) + 1);
  strcpy(plugin->path, path);
  
  plugin->displayName = malloc(strlen(displayName) + 1);
  strcpy(plugin->displayName, displayName);
  
  plugin->handle = handle;
  plugin->unload = NULL;
  
  loadingPlugin = pluginCount;
  registerFailed = 0;
  
  *firstNative = nativeCount;
  
  int status = load(&pluginApi);
  
  loadingPlugin = -1;
  
  if(registerFailed) {
    snprintf(error, errorSize, "Plugin \"%s\": %s", displayName, registerError);
    status = -1;
  } else if(status != 0) {
    snprintf(error, errorSize, "Plugin \"%s\" could not start (elp_Load returned %d)", displayName, status);
  }
  
  if(status != 0) {
    // Forget the natives it registered before failing
    for(int i = *firstNative; i < nativeCount; i++) {
      free(natives[i].name);
      free(natives[i].paramTypes);
    }
    
    nativeCount = *firstNative;
    
    free(plugin->path);
    free(plugin->displayName);
    
    CloseLibrary(handle);
    return PLUGIN_FAILED;
  }
  
  plugin->unload = unload;
  pluginCount++;
  
  if(!unloadRegistered) {
    atexit(UnloadAllPlugins);
    unloadRegistered = 1;
  }
  
  return PLUGIN_LOADED;
}

int PluginNativeCount(void) {
  return nativeCount;
}

const PluginNative* PluginNativeAt(int index) {
  return &natives[index];
}

const char* PluginDisplayName(int pluginIndex) {
  return plugins[pluginIndex].displayName;
}

int PluginCallNative(int index, const VarType* types, const double* numbers,
  const int* isNone, char* const* strings, PluginResult* result, char* error, int errorSize) {
  const PluginNative* native = &natives[index];
  
  ELCall call;
  
  memset(&call, 0, sizeof(call));
  
  call.native = native;
  call.types = types;
  call.numbers = numbers;
  call.isNone = isNone;
  call.strings = strings;
  
  memset(result, 0, sizeof(*result));
  
  native->function(&call);
  
  if(call.failed) {
    snprintf(error, errorSize, "%s", call.error);
    free(call.retString);
    return 0;
  }
  
  // A function that has a result but never gave one returns NONE, like an
  // EL function that reaches its end without 'return'
  if(native->hasReturn) {
    if(!call.returned || call.retIsNone) {
      result->isNone = 1;
      
      if(native->returnType == VAR_STR) {
        result->string = malloc(1);
        result->string[0] = '\0';
      }
    } else if(native->returnType == VAR_STR) {
      result->string = call.retString;
      call.retString = NULL;
    } else if(native->returnType == VAR_INT) {
      // An EL int is a 32 bit number, a bigger value would be changed without
      // anyone noticing, so it is an error instead
      if(!(call.retNumber >= (double) INT_MIN && call.retNumber <= (double) INT_MAX)) {
        snprintf(error, errorSize, "Plugin function '%s' returned %.0f, which does not fit in an EL int (32 bit)",
          native->name, call.retNumber);
        free(call.retString);
        return 0;
      }
      
      result->number = (double) (long long) call.retNumber;
    } else if(native->returnType == VAR_BOOL) {
      result->number = call.retNumber != 0 ? 1 : 0;
    } else {
      result->number = call.retNumber;
    }
  }
  
  free(call.retString);
  return 1;
}
