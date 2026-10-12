// Needed for realpath() and readlink() while compiling with -std=c99
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "source.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

// Size of the buffers used to read paths from the operating system
#define PATH_BUFFER_SIZE 4096

// One registered source file
typedef struct {
  // Path the file is opened from
  char* openPath;
  
  // Name shown in error messages
  char* displayName;
  
  // Real path with links and "./" removed, used to detect a file twice
  char* canonicalPath;
} SourceFile;

// Registered files, indexed by file id. Grows as needed.
SourceFile* sourceFiles;
int sourceFileCount = 0;
int sourceFileCapacity = 0;

// Full path of the elvm executable, set by SetExecutablePath
char* executablePath = NULL;

// Folder of the elvm executable, built the first time it is asked for
char* executableDirectory = NULL;

int MakeLocation(int fileId, int line) {
  return (fileId << LOCATION_LINE_BITS) | line;
}

int LocationFileId(int location) {
  return location >> LOCATION_LINE_BITS;
}

int LocationLine(int location) {
  return location & LOCATION_LINE_MASK;
}

const char* LocationFile(int location) {
  return SourceDisplayName(LocationFileId(location));
}

// Resolve a path to one unique spelling, so the same file is never taken for
// two different ones. If the file cannot be resolved the path is kept as is.
char* CanonicalPath(const char* path) {
  char* resolved = NULL;

#ifdef _WIN32
  resolved = _fullpath(NULL, path, 0);
  
  // Windows paths are not case sensitive
  if(resolved != NULL) {
    for(char* c = resolved; *c != '\0'; c++) {
      *c = (char) tolower((unsigned char) *c);
    }
  }
#else
  resolved = realpath(path, NULL);
#endif
  
  if(resolved == NULL) {
    return DupString(path);
  }
  
  return resolved;
}

int AddSourceFile(const char* openPath, const char* displayName) {
  if(sourceFileCount >= MAX_SOURCE_FILES) {
    return -1;
  }
  
  if(sourceFileCount >= sourceFileCapacity) {
    sourceFileCapacity = sourceFileCapacity == 0 ? 8 : sourceFileCapacity * 2;
    sourceFiles = realloc(sourceFiles, sourceFileCapacity * sizeof(SourceFile));
  }
  
  sourceFiles[sourceFileCount].openPath = DupString(openPath);
  sourceFiles[sourceFileCount].displayName = DupString(displayName);
  sourceFiles[sourceFileCount].canonicalPath = CanonicalPath(openPath);
  
  return sourceFileCount++;
}

int FindSourceFile(const char* openPath) {
  char* canonical = CanonicalPath(openPath);
  
  int found = -1;
  
  for(int i = 0; i < sourceFileCount; i++) {
    if(strcmp(sourceFiles[i].canonicalPath, canonical) == 0) {
      found = i;
      break;
    }
  }
  
  free(canonical);
  
  return found;
}

const char* SourceOpenPath(int fileId) {
  if(fileId < 0 || fileId >= sourceFileCount) {
    return "?";
  }
  
  return sourceFiles[fileId].openPath;
}

const char* SourceDisplayName(int fileId) {
  if(fileId < 0 || fileId >= sourceFileCount) {
    return "?";
  }
  
  return sourceFiles[fileId].displayName;
}

char* ReadFileText(const char* path) {
  FILE* file = fopen(path, "r");
  
  if(file == NULL) {
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  
  long size = ftell(file);
  
  rewind(file);
  
  if(size < 0) {
    fclose(file);
    return NULL;
  }
  
  char* buffer = malloc(size + 1);
  
  // Text mode can read fewer bytes than the file size (line endings)
  size_t count = fread(buffer, 1, size, file);
  
  buffer[count] = '\0';
  
  fclose(file);
  
  return buffer;
}

int SourceTextFits(const char* text) {
  int lines = 1;
  
  for(const char* c = text; *c != '\0'; c++) {
    if(*c == '\n') {
      lines++;
    }
  }
  
  return lines < LOCATION_LINE_MASK;
}

// True for '/' and '\'
int IsSeparator(char c) {
  return c == '/' || c == '\\';
}

int IsAbsolutePath(const char* path) {
  // "/dir", "\dir"
  if(IsSeparator(path[0])) {
    return 1;
  }
  
  // "C:dir", "C:\dir"
  if(((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) &&
     path[1] == ':') {
    return 1;
  }
  
  return 0;
}

char* JoinPath(const char* directory, const char* name) {
  // Nothing to join with
  if(directory[0] == '\0') {
    return DupString(name);
  }
  
  int length = (int) strlen(directory);
  int needsSeparator = !IsSeparator(directory[length - 1]);
  
  char* joined = malloc(length + strlen(name) + 2);
  
  strcpy(joined, directory);
  
  if(needsSeparator) {
    strcat(joined, "/");
  }
  
  strcat(joined, name);
  
  return joined;
}

char* DirectoryOfPath(const char* path) {
  int lastSeparator = -1;
  
  for(int i = 0; path[i] != '\0'; i++) {
    if(IsSeparator(path[i])) {
      lastSeparator = i;
    }
  }
  
  // No folder in the path
  if(lastSeparator == -1) {
    return DupString("");
  }
  
  // File directly in the root folder, keep the root itself
  if(lastSeparator == 0) {
    return DupString("/");
  }
  
  char* directory = malloc(lastSeparator + 1);
  
  memcpy(directory, path, lastSeparator);
  
  directory[lastSeparator] = '\0';
  
  return directory;
}

char* NormalizePath(const char* path) {
  int length = (int) strlen(path);
  
  char* result = malloc(length + 1);
  
  // Start index in 'result' of every part kept so far
  int* partStarts = malloc((length + 1) * sizeof(int));
  int partCount = 0;
  int resultLength = 0;
  
  // Keep a leading separator, an absolute path stays absolute
  if(length > 0 && IsSeparator(path[0])) {
    result[resultLength++] = '/';
  }
  
  int index = 0;
  
  while(index < length) {
    // Skip separators between parts
    while(index < length && IsSeparator(path[index])) {
      index++;
    }
    
    int partStart = index;
    
    while(index < length && !IsSeparator(path[index])) {
      index++;
    }
    
    int partLength = index - partStart;
    
    if(partLength == 0 || (partLength == 1 && path[partStart] == '.')) {
      continue;
    }
    
    // "folder/.." cancels out, unless the part before it is also ".."
    if(partLength == 2 && path[partStart] == '.' && path[partStart + 1] == '.' && partCount > 0 &&
       strncmp(&result[partStarts[partCount - 1]], "..", 2) != 0) {
      partCount--;
      resultLength = partStarts[partCount];
      
      // Also drop the separator that was placed before the removed part
      if(partCount > 0) {
        resultLength--;
      }
      
      continue;
    }
    
    if(partCount > 0) {
      result[resultLength++] = '/';
    }
    
    partStarts[partCount++] = resultLength;
    
    memcpy(&result[resultLength], &path[partStart], partLength);
    
    resultLength += partLength;
  }
  
  result[resultLength] = '\0';
  
  free(partStarts);
  
  // Everything cancelled out
  if(resultLength == 0) {
    free(result);
    
    return DupString(".");
  }
  
  return result;
}

int IsSafeRelativePath(const char* path) {
  if(path[0] == '\0' || IsAbsolutePath(path)) {
    return 0;
  }
  
  // Check every part between separators
  int partStart = 0;
  
  for(int i = 0; ; i++) {
    if(path[i] == ':') {
      return 0;
    }
    
    if(IsSeparator(path[i]) || path[i] == '\0') {
      int partLength = i - partStart;
      
      if(partLength == 0 && path[i] != '\0') {
        return 0;
      }
      
      if(partLength == 2 && path[partStart] == '.' && path[partStart + 1] == '.') {
        return 0;
      }
      
      if(path[i] == '\0') {
        break;
      }
      
      partStart = i + 1;
    }
  }
  
  return 1;
}

int IsValidLibraryName(const char* name) {
  if(name[0] == '\0' || name[0] == '.') {
    return 0;
  }
  
  for(int i = 0; name[i] != '\0'; i++) {
    char c = name[i];
    
    if(!((c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '_' || c == '-' || c == '.')) {
      return 0;
    }
  }
  
  return 1;
}

void SetExecutablePath(const char* argv0) {
  char buffer[PATH_BUFFER_SIZE];
  
  buffer[0] = '\0';
  
  // Ask the operating system where the executable really is. argv[0] cannot
  // be trusted for this: it may be a bare name found through PATH.
#ifdef _WIN32
  DWORD length = GetModuleFileNameA(NULL, buffer, PATH_BUFFER_SIZE);
  
  if(length == 0 || length >= PATH_BUFFER_SIZE) {
    buffer[0] = '\0';
  }
#elif defined(__APPLE__)
  unsigned int size = PATH_BUFFER_SIZE;
  
  if(_NSGetExecutablePath(buffer, &size) != 0) {
    buffer[0] = '\0';
  }
#elif defined(__linux__)
  ssize_t length = readlink("/proc/self/exe", buffer, PATH_BUFFER_SIZE - 1);
  
  if(length > 0) {
    buffer[length] = '\0';
  }
#endif
  
  // Fall back to argv[0] on systems without a way to ask
  if(buffer[0] == '\0' && argv0 != NULL) {
    strncpy(buffer, argv0, PATH_BUFFER_SIZE - 1);
    
    buffer[PATH_BUFFER_SIZE - 1] = '\0';
  }
  
  free(executablePath);
  free(executableDirectory);
  
  executablePath = DupString(buffer);
  executableDirectory = NULL;
}

const char* GetExecutableDirectory(void) {
  if(executableDirectory == NULL) {
    char* directory = DirectoryOfPath(executablePath == NULL ? "" : executablePath);
    
    // No folder in the path means the current folder
    if(directory[0] == '\0') {
      free(directory);
      
      directory = DupString(".");
    }
    
    executableDirectory = directory;
  }
  
  return executableDirectory;
}

char* GetLibraryDirectory(const char* name) {
  char* libraries = JoinPath(GetExecutableDirectory(), "library");
  char* directory = JoinPath(libraries, name);
  
  free(libraries);
  
  return directory;
}
