#include "common.hpp"

namespace rotate
{

struct compile_options
{
    const s32 argc;
    char **const argv;
    const char *filename = NULL;
    bool debug_info      = false;
    bool debug_symbols   = false;
    bool timer           = false;
    bool lex_only        = false;

    compile_options(const s32 argc, char **argv) : argc(argc), argv(argv)
    {
        filename = argv[1];
        for (s32 i = 2; i < argc; i++)
        {
            auto string = argv[i];

            if (strcmp(string, "--log") == 0)
            {
                debug_info = true;
            }
            else if (strcmp(string, "--timer") == 0)
            {
                timer = true;
            }
            else if (strcmp(string, "--lex-only") == 0)
            {
                lex_only = true;
            }
            else
            {
                log_error_unknown_flag(string);
            }
        }
    }

    ~compile_options() = default;

    void log_error_unknown_flag(const char *str);
};

//
int compile(compile_options *options) noexcept;

} // namespace rotate
