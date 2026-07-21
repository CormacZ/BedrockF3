-- scripts/get-version-info.lua
-- Read version information from the most recent git tag and short SHA.

-- Helper: run a command and return its trimmed stdout, or nil on failure.
-- xmake's os.iorunv throws on non-zero exit, so we wrap it in pcall.
local function try_runv(argv)
    local ok, out = pcall(os.iorunv, "git", argv)
    if not ok or out == nil then
        return nil
    end
    return tostring(out):gsub("^%s+", ""):gsub("%s+$", "")
end

function get_version_info()
    local version_str = "0.0.0"
    local version     = { major = 0, minor = 0, patch = 0 }
    local short_sha   = "dev"

    -- Read the most recent tag, e.g. "v0.1.0" or "0.1.0". A repo with no
    -- tags returns nil, in which case we keep the 0.0.0 fallback.
    local tag = try_runv({ "describe", "--tags", "--abbrev=0" })
    if tag then
        local major, minor, patch = tag:match("v?(%d+)%.(%d+)%.(%d+)")
        if major then
            version = { major = tonumber(major), minor = tonumber(minor), patch = tonumber(patch) }
            version_str = string.format("%d.%d.%d", version.major, version.minor, version.patch)
        end
    end

    local sha = try_runv({ "rev-parse", "--short", "HEAD" })
    if sha then
        short_sha = sha
    end

    return {
        version_str = version_str,
        major       = version.major,
        minor       = version.minor,
        patch       = version.patch,
        short_sha   = short_sha,
    }
end
