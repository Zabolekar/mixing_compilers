# What Could Go Wrong If You Mix C Compilers

On Windows, your dependencies often consist of headers and already compiled DLLs. The source code might not be available, or it might be available but you don't feel like compiling everything yourself. A common expectation is that a C library is a C library and it doesn't matter what compiler it has been compiled with. Sadly, it does.

## Real Life Example

The `char *fftw_export_wisdom_to_string(void)` function from [FFTW](https://www.fftw.org/) allocates a string, and the caller is responsible for `free`ing it when it's no longer needed. On Windows, if FFTW has been compiled with GCC and the program that uses it has been compiled with MSVC, your program will work until it calls this function, and then it will crash.

Compiling FFTW takes time and effort, so I'll continue with a minimal example instead.

## Minimal Example

You'll need x64 Windows, GCC, e.g. built by [Strawberry Perl project](https://strawberryperl.com), the MSVC compiler toolset and the Clang version that comes with it. Visual Studio is not needed.

The required files are (you can clone them from https://github.com/Zabolekar/mixing_compilers):

- `README.md`, this is what you're reading right now.

- `wrapper.c` and `wrapper.h`, a trivial wrapper around `malloc`:

```c
// wrapper.h:
__declspec (dllexport)
void *malloc_wrapper(size_t);

// wrapper.c:
#include <stdlib.h>
#include "wrapper.h"

void *malloc_wrapper(size_t size)
{
    return malloc(size);
}
```

- `wrapper.def`, which we'll need to generate an import library manually (see below):

```
EXPORTS
malloc_wrapper
```

- `main.c`, which calls the malloc wrapper:

```c
#include <stdlib.h>
#include "wrapper.h"

int main()
{
    void *p = malloc_wrapper(sizeof(int));
    free(p);
}
```

- `clean.bat`, which you should call to delete the generated files from an old test before running the next test:

```bat
del *.dll *.lib *.exp *.exe *.obj
```

First, we'll verify that everything works if you don't mix compilers.

Compiling with GCC:

```bat
gcc wrapper.c -shared -o wrapper.dll
gcc main.c wrapper.dll -o main.exe
main.exe
echo %errorlevel%
```

Output: `0`.

Compiling with MSVC (assuming everything has already been configured and `vcvars64.bat` has been called):

```bat
cl wrapper.c /LD
cl main.c wrapper.lib
main.exe
echo %errorlevel%
```

Output: `0`.

Note that GCC links with the DLL itself and MSVC needs a `.lib` file. GCC can generate `.lib` files, too, but by default it doesn't. Because we simulate a sutuation where the library has already been compiled by someone else, we generate the `.lib` file with a separate tool.

Knowing all that, let's compile the DLL with GCC and the caller with MSVC:

```bat
gcc wrapper.c -shared -o wrapper.dll
lib /def:wrapper.def /out:wrapper.lib /machine:x64
cl main.c wrapper.lib
main.exe
echo %errorlevel%
```
Output: `-1073740940`, that is, `0xc0000374`, also known as `STATUS_HEAP_CORRUPTION`.

Same in the other direction:

```bat
cl wrapper.c /LD
gcc main.c wrapper.dll -o main.exe
main.exe
echo %errorlevel%
```

Output: `-1073740940`.

## Target Triplets

A useful term to talk about this kind of incompatibilities is *target triplets*, convenient names to describe what environment we are building for. The name "triplets" doesn't mean that they always consist of three parts. In our case, they do, but it's an accident.

An easy way to experiment with them is by using Clang and its `-target` option. This allows us to generate DLLs that can be used with GCC or DLLs that can be used with MSVC:

```bat
clang wrapper.c -shared -o wrapper.dll -target x86_64-windows-gnu
gcc main.c wrapper.dll -o main.exe
main.exe
echo %errorlevel%
```

Output: `0`.

```bat
clang wrapper.c -shared -o wrapper.dll -target x86_64-windows-msvc
cl main.c wrapper.lib
main.exe
echo %errorlevel%
```

Output: `0`, also note that this time Clang generates the `.lib` file by default.

You can also verify that the `x86_64-windows-gnu` DLL causes a crash when used with MSVC and the `x86_64-windows-msvc` DLL causes a crash when used with GCC.

## Open Questions

Can you, by looking at a compiled DLL, find out how it's been compiled and whether it's safe to link against it with your current settings? I don't think it's possible, but maybe I'm wrong.
