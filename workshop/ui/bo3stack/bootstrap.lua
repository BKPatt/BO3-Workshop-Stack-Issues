-- Loaded by the front-end only when BO3 Workshop Stack is selected in MODS.
-- Uses HavokScript's native module API; no external process or download.
local ok, problem = pcall(function()
    local selected = Engine.UsingModsUgcName()
    local directory
    if selected == "bo3_workshop_stack" then
        directory = "./mods/bo3_workshop_stack/zone/"
    elseif type(selected) == "string" and string.match(selected, "^%d+$") then
        directory = "../../workshop/content/311210/" .. selected .. "/"
    else
        error("Cannot identify this mod's installed folder: " .. tostring(selected))
    end
    local packages = require("package")
    if type(packages.loadlib) ~= "function" then
        error("Native Lua modules are unavailable in this game client.")
    end
    local initialize, loadProblem = packages.loadlib(directory .. "bo3_stack.dll", "BO3Stack_Initialize")
    if type(initialize) ~= "function" then
        error("Cannot load bo3_stack.dll from " .. directory .. ". Let Steam finish downloading this item. Native module error: " .. tostring(loadProblem))
    end
    if initialize() ~= true then
        -- The native entry shows the exact failure without accessing an
        -- unsupported Lua VM. Avoid replacing it with a second generic error.
        return
    end
end)
if not ok then
    Engine.ComError(Enum.errorCode.ERROR_UI, "BO3 Workshop Stack: " .. tostring(problem))
end
