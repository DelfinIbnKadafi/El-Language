#ifndef SOURCE_H
#define SOURCE_H

// A "location" is one int that carries both the source file and the line, so
// every token and instruction can report errors against the right file, even
// when code is pulled in from other files with module / library.
//
//   location = (file id << LOCATION_LINE_BITS) | line
//
// The main file always has id 0, so its locations are plain line numbers.
#define LOCATION_LINE_BITS 20
#define LOCATION_LINE_MASK ((1 << LOCATION_LINE_BITS) - 1)

// Highest number of source files in one program (31 bits keep the int positive)
#define MAX_SOURCE_FILES (1 << (31 - LOCATION_LINE_BITS))

// Build a location from a file id and a line
int MakeLocation(int fileId, int line);

// Split a location back into its file id and its line
int LocationFileId(int location);
int LocationLine(int location);

// Display name of the file a location belongs to, used in error messages
const char* LocationFile(int location);

// Register a source file. 'openPath' is where it is opened from, 'displayName'
// is how it appears in error messages. Returns its file id, or -1 if
// MAX_SOURCE_FILES is reached.
int AddSourceFile(const char* openPath, const char* displayName);

// Find an already registered file by the path it is opened from, comparing the
// real files (so "a.ell" and "./a.ell" match). Returns its id, or -1.
int FindSourceFile(const char* openPath);

// Path a registered file was opened from, and its display name
const char* SourceOpenPath(int fileId);
const char* SourceDisplayName(int fileId);

// Read a whole text file onto the heap, or NULL if it cannot be opened
char* ReadFileText(const char* path);

// True if the text has few enough lines to be given locations
int SourceTextFits(const char* text);

// Path helpers. Both separators ('/' and '\') are understood on every system.
// JoinPath and DirectoryOfPath return new heap strings.
int IsAbsolutePath(const char* path);
char* JoinPath(const char* directory, const char* name);
char* DirectoryOfPath(const char* path);

// Tidy a path for display: removes "." parts and folds "folder/.." away, so
// "sub/../a.ell" is shown as "a.ell". Returns a new heap string.
char* NormalizePath(const char* path);

// True if a path stays inside its folder: not absolute, not empty, no ".."
int IsSafeRelativePath(const char* path);

// True if the text can be used as a library name (it becomes a folder name)
int IsValidLibraryName(const char* name);

// Remember where the elvm executable is. argv0 is only a fallback, the real
// location is asked from the operating system when possible.
void SetExecutablePath(const char* argv0);

// Folder that contains the elvm executable (not the working directory)
const char* GetExecutableDirectory(void);

// Folder of a library: "<folder of elvm>/library/<name>"
char* GetLibraryDirectory(const char* name);

#endif
