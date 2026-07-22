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
