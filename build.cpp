#define EZBUILD_IMPLEMENTATION
#include "deps/ezbuild/ezbuild.hpp"

using namespace Sl;

const char* RELEASE_CMD_ARG = "release";
const char* DEBUG_EXE = "debug/real_engine";
const char* RELEASE_EXE = "release/real_engine";
const char* DEBUG_OUTPUT_DIR = "debug/output";
const char* RELEASE_OUTPUT_DIR = "release/output";
const char* THREAD_COUNT_DEF_FMT = "THREAD_COUNT=%d";
const char* PAGE_SIZE_DEF_FMT = "PAGE_SIZE=%d";

int main(int argc, char **argv)
{
    bool is_release = false;
    if (argc > 1)
    {
        is_release = memcmp(argv[1], "release", 5) == 0;
    }

    SystemInfo sys_info = get_system_info();
    StrBuilder thread_count_define;
    thread_count_define.appendf(THREAD_COUNT_DEF_FMT, sys_info.number_of_processors);
    StrBuilder page_size_define;
    page_size_define.appendf(PAGE_SIZE_DEF_FMT, sys_info.page_size);

    if (!is_release) log_info("Starting debug build...\n");
    else log_info("Starting release build...\n");

    FlagsOptimization optimization = is_release ? FlagsOptimization::SPEED : FlagsOptimization::NONE;
    FlagsSTD std = FlagsSTD::CPP20;

    rebuild_itself((ExecutableOptions{ .debug = !is_release, .optimize = optimization, .std = std }),
                    argc, argv, "deps/ezbuild/ezbuild.hpp");

    // Check if this build.cpp script was rebuilt.
    bool force_rebuilt = was_script_rebuilt(argc, argv);

    // Create cmd object in which we gonna configure our build...
    Cmd cmd {};
    
    cmd.start_build({
        .is_cpp = true,
        .incremental_build = true,
        .optimize = optimization,
        .warnings = FlagsWarning::ALL,
        .std = std
    });

    cmd.add_include_path("deps/");
    cmd.add_include_path("src");
    cmd.include_sources_from_folder("src");
    cmd.include_sources_from_folder("src/core");
    cmd.include_sources_from_folder("src/collections");

    LocalArray<StrView> shared_flags;
    shared_flags.push("-march=native");
    shared_flags.push("-fno-rtti");

    LocalArray<StrView> debug_flags;
    debug_flags.push("-g");
    debug_flags.push("-O0");

    LocalArray<StrView> release_flags;
    release_flags.push("-flto");

    LocalArray<StrView> debug_defines;
    debug_defines.push("RG_DEBUG");

    LocalArray<StrView> release_defines;
    release_defines.push("RG_RELEASE");

    for (auto flag : shared_flags)
    {
        cmd.add_cpp_flag(flag);
    }

    if (!is_release)
    {
        for (auto flag : debug_flags)
        {
            cmd.add_cpp_flag(flag);
        }
        for (auto define : debug_defines)
        {
            cmd.add_define(define);
        }
    }
    else
    {
        for (auto flag : release_flags)
        {
            cmd.add_cpp_flag(flag);
        }
        for (auto define : release_defines)
        {
            cmd.add_define(define);
        }
    }

    // shared defines.
    cmd.add_define(thread_count_define.to_string_view());
    cmd.add_define(page_size_define.to_string_view());

    if (is_release) cmd.output_file(RELEASE_EXE);
    else cmd.output_file(DEBUG_EXE);

    if (is_release) cmd.output_folder(RELEASE_OUTPUT_DIR);
    else cmd.output_folder(DEBUG_OUTPUT_DIR);

    bool run = false;
    if (!cmd.end_build(run, force_rebuilt))
        return EXIT_FAILURE;
}
