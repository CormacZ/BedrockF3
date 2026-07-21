-- scripts/generate-manifest.lua
-- Generate a manifest.json for a LeviLamina native mod.

local json = import("scripts.json", { rootdir = os.projectdir() })

function generate_manifest(output_path, info)
    local manifest = {
        name     = info.name,
        entry    = info.entry,
        version  = info.version,
        type     = "native",
        platform = info.platform or "win-x64",
    }

    local f = io.open(output_path, "w")
    if not f then
        raise("failed to open %s for writing", output_path)
    end
    f:write(json.encode(manifest))
    f:close()
end
