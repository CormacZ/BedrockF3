-- scripts/get-version-info.lua
-- Read version information from the VERSION file in the project root.
--
-- Why not git tags? xmake deliberately disables Lua's pcall/xpcall and
-- provides try/catch as a custom parser feature. But try/catch is only
-- available in xmake's own script context (xmake.lua), not in scripts
-- loaded via import(). os.iorunv throws on non-zero exit, so a repo
-- with no tags (or git unavailable) would crash the build. Reading from
-- a static VERSION file avoids the issue entirely.
--
-- Bump the version in VERSION, or have a release-time script rewrite
-- the file from `git describe --tags --abbrev=0`, when you want
-- git-based versioning.

local function read_file(path)
    local f = io.open(path, "r")
    if not f then
        return nil
    end
    local content = f:read("*a")
    f:close()
    return content
end

function get_version_info()
    local version_str = (read_file(path.join(os.projectdir(), "VERSION")) or "0.0.0")
    version_str = version_str:gsub("^%s+", ""):gsub("%s+$", "")
    local major, minor, patch = version_str:match("v?(%d+)%.(%d+)%.(%d+)")
    if not major then
        major, minor, patch = 0, 0, 0
        version_str = "0.0.0"
    end

    return {
        version_str = version_str,
        major       = tonumber(major),
        minor       = tonumber(minor),
        patch       = tonumber(patch),
        short_sha   = "dev",
    }
end
