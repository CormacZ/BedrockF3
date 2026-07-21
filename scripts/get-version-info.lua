-- scripts/get-version-info.lua
-- Read version information from the most recent git tag and short SHA.

function get_version_info()
    local version_str = "0.0.0"
    local version     = { major = 0, minor = 0, patch = 0 }

    -- Read the most recent tag, e.g. "v0.1.0" or "0.1.0".
    local tag = os.iorunv("git", { "describe", "--tags", "--abbrev=0" })
    if tag then
        tag = tag:gsub("^%s+", ""):gsub("%s+$", "")
        local major, minor, patch = tag:match("v?(%d+)%.(%d+)%.(%d+)")
        if major then
            version = { major = tonumber(major), minor = tonumber(minor), patch = tonumber(patch) }
            version_str = string.format("%d.%d.%d", version.major, version.minor, version.patch)
        end
    end

    local short_sha = "dev"
    local sha = os.iorunv("git", { "rev-parse", "--short", "HEAD" })
    if sha then
        short_sha = sha:gsub("^%s+", ""):gsub("%s+$", "")
    end

    return {
        version_str = version_str,
        major       = version.major,
        minor       = version.minor,
        patch       = version.patch,
        short_sha   = short_sha,
    }
end
