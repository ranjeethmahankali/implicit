#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

static int get_absolute_path(char const* relative, char* absolute, size_t absPathSize);

static std::string load_source(const char* filename);

namespace cl_kernel_sources
{
    std::string render_kernel();
    std::string abs_path();
}
