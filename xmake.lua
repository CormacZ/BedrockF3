add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

set_xmakever("3.0.0")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

-- Dependency declared at root scope (xmake requires this; it cannot be called inside a target).
add_requires("levilamina", { configs = { target_type = "client" } })

target("BedrockF3")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_shflags("/DELAYLOAD:bedrock_runtime.dll")
    add_defines(
        "NOMINMAX",
        "UNICODE",
        "_HAS_CXX23=1",
        "LL_PLAT_C"
    )
    add_cxflags(
        "/EHs",
        "-Wno-microsoft-cast",
        "-Wno-invalid-offsetof",
        "-Wno-c++2b-extensions",
        "-Wno-microsoft-include",
        "-Wno-overloaded-virtual",
        "-Wno-ignored-qualifiers",
        "-Wno-missing-field-initializers",
        "-Wno-potentially-evaluated-expression",
        "-Wno-pragma-system-header-outside-header",
        { tools = { "clang_cl" } }
    )
    set_toolchains("clang-cl")

    -- Client-side build is the default; the only platform this mod supports today.
    add_packages("levilamina")

    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    set_optimize("aggressive")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_headerfiles("src/**.h")

    -- The system_info module uses DXGI (IDXGIAdapter::GetDesc1) for
    -- GPU name detection. dxgi.lib + dxguid.lib are part of the
    -- Windows SDK and are auto-found by clang-cl, but we name them
    -- explicitly so xmake's dependency analyser is happy. Only
    -- needed on Windows (which is the only platform we build for).
    if is_plat("windows") then
        add_links("dxgi", "dxguid")
    end

    before_link(function(target)
        import("lib.detect.find_file")
        import("core.project.config")

        os.addenvs(target:pkgenvs())

        local libdir = path.join(config.builddir(), ".prelink", "lib")
        if os.exists(libdir) then os.rm(libdir) end
        os.mkdir(libdir)

        local data  = assert(find_file("bedrock_runtime_data", { "$(env PATH)" }), "Cannot find bedrock_runtime_data")
        local link  = assert(find_file("prelink.exe",          { "$(env PATH)" }), "Cannot find prelink.exe")

        os.runv(link, {
            string.format("client-%s-%s", target:plat(), target:arch()),
            path.join(config.builddir(), ".prelink"),
            data,
            table.unpack(target:objectfiles())
        })

        target:add("linkdirs", libdir)
        target:add("links", "bedrock_runtime_api")
    end)

    after_build(function(target)
        local output_dir = path.join(os.projectdir(), "bin", target:name())
        os.rm(output_dir)

        os.vcp(target:targetfile(),  format("%s/", output_dir))
        os.vcp(target:symbolfile(),  format("%s/", output_dir))

        import("scripts.generate-manifest", { rootdir = os.projectdir() }).generate_manifest(
            format("%s/manifest.json", output_dir),
            {
                name     = target:name(),
                entry    = path.basename(target:targetfile()),
                version  = import("scripts.get-version-info", { rootdir = os.projectdir() }).get_version_info().version_str,
                platform = "client"
            }
        )
    end)
end

-- =============================================================================
-- Sanitizer build (ASan + UBSan)
-- =============================================================================
-- Separate target that compiles the same source files with
-- -fsanitize=address,undefined -g. Used as a CI build check to
-- catch issues at compile time (sanitizer-incompatible code,
-- missing annotations, link-time ASan runtime resolution).
--
-- For runtime ASan testing of the actual loaded DLL, the user
-- needs to:
--   1. Build this target: xmake build BedrockF3-asan
--   2. Copy bin/BedrockF3-asan/BedrockF3-asan.dll into the
--      LeviLauncher plugins/ directory.
--   3. Make sure clang_rt.asan_dynamic-x86_64.dll is on the
--      PATH (it ships with the LLVM toolchain installed by
--      the github action).
--
-- set_default(false) means a bare `xmake build` skips this
-- target -- only the main BedrockF3 DLL is built by default.
-- To build the ASan DLL, pass the target name explicitly:
--   xmake build BedrockF3-asan
--
-- BedrockF3 does not have unit tests yet. Once doctest is
-- added (per the dev-tools research), a third target
-- BedrockF3-tests will run the same source compiled with
-- sanitizers plus the test binary.
if is_plat("windows") then
target("BedrockF3-asan")
    set_default(false)
    add_cxflags(
        "/EHa",
        "/utf-8",
        "-fsanitize=address",
        "-fsanitize=undefined",
        "-fno-omit-frame-pointer",
        "-g"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_runtime.dll",
        "-fsanitize=address",
        "-fsanitize=undefined"
    )
    add_defines(
        "NOMINMAX",
        "UNICODE",
        "_HAS_CXX23=1",
        "LL_PLAT_C"
    )
    set_toolchains("clang-cl")
    add_packages("levilamina")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    set_optimize("aggressive")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_links("dxgi", "dxguid")
    before_link(function(target)
        import("lib.detect.find_file")
        import("core.project.config")

        os.addenvs(target:pkgenvs())

        local libdir = path.join(config.builddir(), ".prelink", "lib")
        if os.exists(libdir) then os.rm(libdir) end
        os.mkdir(libdir)

        local data  = assert(find_file("bedrock_runtime_data", { "$(env PATH)" }), "Cannot find bedrock_runtime_data")
        local link  = assert(find_file("prelink.exe",          { "$(env PATH)" }), "Cannot find prelink.exe")

        os.runv(link, {
            string.format("client-%s-%s", target:plat(), target:arch()),
            path.join(config.builddir(), ".prelink"),
            data,
            table.unpack(target:objectfiles())
        })

        target:add("linkdirs", libdir)
        target:add("links", "bedrock_runtime_api")
    end)
    after_build(function(target)
        local output_dir = path.join(os.projectdir(), "bin", target:name())
        os.rm(output_dir)

        os.vcp(target:targetfile(),  format("%s/", output_dir))
        os.vcp(target:symbolfile(),  format("%s/", output_dir))
    end)
end
end
