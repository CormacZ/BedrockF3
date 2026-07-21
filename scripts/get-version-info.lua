-- scripts/get-version-info.lua
-- Read version information from the most recent git tag and short SHA.

function get_version_info()
    local version_str = "0.0.0"
    local version     = { major = 0, minor = 0, patch = 0 }
    local short_sha   = "dev"

    -- xmake's os.iorunv throws an exception when the command exits
    -- non-zero. Wrap each git invocation in try-catch so a repo with no
    -- tags (e.g. the first build before a release tag is pushed) still
    -- succeeds, falling back to 0.0.0 / "dev" instead of aborting the
    -- whole build.

    try
    function()
        local tag = os.iorunv("git", { "describe", "--tags", "--abbrev=0" })
        if tag then
            tag = tag:gsub("^%s+", ""):gsub("%s+$", "")
            local major, minor, patch = tag:match("v?(%d+)%.(%d+)%.(%d+)")
            if major then
                version = { major = tonumber(major), minor = tonumber(minor), patch = tonumber(patch) }
                version_str = string.format("%d.%d.%d", version.major, version.minor, version.patch)
            end
        end
    end
    catch
    function(errors)
        -- No tags yet (or git not available). Keep the 0.0.0 fallback.
        print("get_version_info: no git tag found, using 0.0.0 (" .. errors .. ")")
    end

    try
    function()
        local sha = os.iorunv("git", { "rev-parse", "--short", "HEAD" })
        if sha then
            short_sha = sha:gsub("^%s+", ""):gsub("%s+$", "")
        end
    end
    catch
    function(errors)
        -- No git history (very unusual). Keep the "dev" fallback.
    end

    return {
        version_str = version_str,
        major       = version.major,
        minor       = version.minor,
        patch       = version.patch,
        short_sha   = short_sha,
    }
end
