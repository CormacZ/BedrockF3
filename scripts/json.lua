-- scripts/json.lua
-- Minimal JSON encoder. Sufficient for generating manifest.json.
--
-- xmake's import() resolves script symbols through the module's global
-- environment, not through the script's return value. Declaring
-- `function encode(...)` (no local) puts the function in the module
-- globals, where the caller can reach it via `import(...).encode(...)`.
-- A trailing `return { encode = ... }` is silently ignored by import.

function encode(value)
    local t = type(value)
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "number" then
        if value ~= value or value == math.huge or value == -math.huge then
            return "null"
        end
        if value == math.floor(value) and math.abs(value) < 1e15 then
            return string.format("%d", value)
        end
        return string.format("%.14g", value)
    elseif t == "string" then
        local out = { '"' }
        for i = 1, #value do
            local c = value:sub(i, i)
            local b = string.byte(c)
            if c == '"' then
                out[#out + 1] = '\\"'
            elseif c == "\\" then
                out[#out + 1] = "\\\\"
            elseif b < 0x20 then
                if c == "\b" then
                    out[#out + 1] = "\\b"
                elseif c == "\f" then
                    out[#out + 1] = "\\f"
                elseif c == "\n" then
                    out[#out + 1] = "\\n"
                elseif c == "\r" then
                    out[#out + 1] = "\\r"
                elseif c == "\t" then
                    out[#out + 1] = "\\t"
                else
                    out[#out + 1] = string.format("\\u%04x", b)
                end
            else
                out[#out + 1] = c
            end
        end
        out[#out + 1] = '"'
        return table.concat(out)
    elseif t == "table" then
        -- Detect array vs object.
        local n = 0
        for _ in pairs(value) do
            n = n + 1
        end
        local is_array = true
        for k in pairs(value) do
            if type(k) ~= "number" then
                is_array = false
                break
            end
        end

        if is_array and n > 0 then
            local parts = {}
            for i = 1, n do
                parts[i] = encode(value[i])
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            for k, v in pairs(value) do
                parts[#parts + 1] = encode(tostring(k)) .. ":" .. encode(v)
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    end
    return "null"
end
