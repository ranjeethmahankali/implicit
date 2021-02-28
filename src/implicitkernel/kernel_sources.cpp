#include <implicitkernel/kernel_sources.h>
#include <algorithm>
#include <filesystem>
#ifdef _MSC_VER
#include <Shlwapi.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#endif

static constexpr const char* renderKernelName = "render.cl";

std::string cl_kernel_sources::render_kernel()
{
    return load_source(renderKernelName);
}

std::string cl_kernel_sources::abs_path()
{
    std::string absPath(MAX_PATH, '\0');
    if (get_absolute_path("", absPath.data(), MAX_PATH) != 0)
    {
        throw "Cannot find absolute path";
    }
    absPath.erase(std::find(absPath.begin(), absPath.end(), '\0'), absPath.end());
    return absPath;
}

int get_absolute_path(char const* relative, char* absolute, size_t absPathSize)
{
#ifdef _MSC_VER
    HMODULE binary = NULL;
    int ret = 0;
    /*First get the handle to this DLL by passing in a pointer to any function inside this dll (cast to a char pointer).*/
    if (!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (char*)&get_absolute_path, &binary))
    {
        /*If we fail to get this handle, NULL will be used as the module, this means GetModuleFileNameA will assume we are
        asking about the current executable. This is harmless for our application because this dll is right next to our application. But this will
        cause the unit tests to fail because the executable in that case is not our application.*/
        ret = GetLastError();
    }
    GetModuleFileNameA(binary, absolute, (DWORD)absPathSize);
    PathRemoveFileSpecA(absolute);
    PathAppendA(absolute, relative);
    return ret;
#else
char result[MAX_PATH];
ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
auto path = std::filesystem::current_path();
if (count != -1) {
    path = std::filesystem::path(result).parent_path() / std::filesystem::path(relative);
}
auto pathstr = path.string();
std::fill(absolute, absolute + MAX_PATH, '\0');
std::copy(pathstr.begin(), pathstr.end(), absolute);
return 0;
#endif
}

std::string load_source(const char* filename)
{
    char absPath[MAX_PATH];
    get_absolute_path(filename, absPath, MAX_PATH);
    std::ifstream f;
    f.open(absPath);
    std::string source;
    if (f.is_open())
    {
        std::ostringstream ss;
        ss << f.rdbuf();
        source.assign(ss.str());
        f.close();
        return source;
    }
    throw "Cannot open file";
}
