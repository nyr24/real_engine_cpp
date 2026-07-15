#define EZBUILD_IMPLEMENTATION
#include "deps/ezbuild/ezbuild.hpp"

using namespace Sl;

const char* RELEASE_CMD_ARG = "release";
const char* DEBUG_EXE = "debug/real_engine";
const char* RELEASE_EXE = "release/real_engine";
const char* DEBUG_OUTPUT_DIR = "debug/output";
const char* RELEASE_OUTPUT_DIR = "release/output";
const char* THREAD_COUNT_DEF_FMT = "__THREAD_COUNT=%d";
const char* PAGE_SIZE_DEF_FMT = "__PAGE_SIZE=%d";

StrView RELEASE_INPUT_OPT = "release";
StrView WARNING_INPUT_OPT = "warn";

int main(int argc, char **argv)
{
    bool is_release = false;
    bool include_warnings = false;

    if (argc > 1)
    {
        is_release = is_argument_set(RELEASE_INPUT_OPT, argc, argv);
        include_warnings = is_argument_set(WARNING_INPUT_OPT, argc, argv);
    }

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
    cmd.include_sources_from_folder("src", true);
    cmd.add_source_file("deps/volk/volk.c");

    LocalArray<StrView> shared_flags;
    shared_flags.push("-march=native");
    shared_flags.push("-fno-rtti");
    shared_flags.push("-fno-exceptions");

    if (include_warnings)
    {
        shared_flags.push("-Wconversion");
        shared_flags.push("-Wsign-conversion");
        shared_flags.push("-Wfloat-conversion");
    }
    // shared_flags.push("-nostdlib++");
    // shared_flags.push("-nostdinc++");

    LocalArray<StrView> debug_flags;
    debug_flags.push("-g");
    debug_flags.push("-O0");

    LocalArray<StrView> release_flags;
    release_flags.push("-flto");

    // Defines.

    SystemInfo sys_info = get_system_info();

    StrBuilder thread_count_define;
    thread_count_define.appendf(THREAD_COUNT_DEF_FMT, sys_info.number_of_processors);

    StrBuilder page_size_define;
    page_size_define.appendf(PAGE_SIZE_DEF_FMT, sys_info.page_size);

    // shared defines.
    cmd.add_define(thread_count_define.to_string_view());
    cmd.add_define(page_size_define.to_string_view());

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
        cmd.add_defines(debug_defines.data(), debug_defines.count());
    }
    else
    {
        for (auto flag : release_flags)
        {
            cmd.add_cpp_flag(flag);
        }
        cmd.add_defines(release_defines.data(), release_defines.count());
    }

    // Libraries.

    if (get_system() == FlagsSystem::WINDOWS)
    {
        LocalArray<StrView> win_libs;
        cmd.add_library_path("./deps/glfw/win32");
        win_libs.push("glfw3.lib");
        cmd.link_libraries_batch(win_libs.data(), win_libs.count());
    }
    else
    {
        LocalArray<StrView> linux_libs;
        cmd.add_library_path("./deps/glfw/linux");
        linux_libs.push("glfw3");
        cmd.link_libraries_batch(linux_libs.data(), linux_libs.count());
    }

    if (is_release) cmd.output_file(RELEASE_EXE);
    else cmd.output_file(DEBUG_EXE);

    if (is_release) cmd.output_folder(RELEASE_OUTPUT_DIR);
    else cmd.output_folder(DEBUG_OUTPUT_DIR);

    bool run = false;
    if (!cmd.end_build(run, force_rebuilt))
        return EXIT_FAILURE;
}
