-- Embedded in the DLL: survives changing the mod that supplies core_mod.ff.
-- This file is installed only in the front-end VM, never in a match or HUD.
if not Engine or Engine.GetCurrentMap() ~= "core_frontend" or not CoD or not CoD.Menu then
    return
end

if not BO3StackUI then
    rawset(_G, "BO3StackUI", {})
end
local S = BO3StackUI

function S.frontend()
    return Engine.GetCurrentMap() == "core_frontend"
end

function S.snapshot()
    if not S.frontend() or not require("bo3stack/state") then
        return nil
    end
    return BO3StackNativeState
end

function S.copy(items)
    local result = {}
    for _, item in ipairs(items) do
        table.insert(result, {id = item.id, title = item.title})
    end
    return result
end

function S.position(items, id)
    for index, item in ipairs(items) do
        if item.id == id then return index end
    end
end

function S.encode(items)
    local ids = {}
    for _, item in ipairs(items) do table.insert(ids, item.id) end
    return table.concat(ids, ",")
end

function S.toggle(draft, item)
    local index = S.position(draft, item.id)
    if index then table.remove(draft, index)
    else table.insert(draft, {id = item.id, title = item.title}) end
end

function S.move(draft, id, delta)
    local index = S.position(draft, id)
    if not index or index + delta < 1 or index + delta > #draft then return false end
    local item = table.remove(draft, index)
    table.insert(draft, index + delta, item)
    return true
end

local function text(parent, value, x, y, width, size, color)
    local label = LUI.UIText.new()
    label:setLeftRight(true, false, x, x + width)
    label:setTopBottom(true, false, y, y + size)
    label:setTTF("fonts/RefrigeratorDeluxe-Regular.ttf")
    label:setAlignment(Enum.LUIAlignment.LUI_ALIGNMENT_LEFT)
    label:setText(value)
    if color then label:setRGB(color[1], color[2], color[3]) end
    parent:addElement(label)
    return label
end

local function panel(parent, x, y, width, height, color, alpha)
    local image = LUI.UIImage.new()
    image:setLeftRight(true, false, x, x + width)
    image:setTopBottom(true, false, y, y + height)
    image:setRGB(color[1], color[2], color[3])
    image:setAlpha(alpha or 1)
    parent:addElement(image)
    return image
end

local function button(parent, title, x, y, width, action)
    local item = LUI.UIElement.new()
    item:setLeftRight(true, false, x, x + width)
    item:setTopBottom(true, false, y, y + 38)
    item:setHandleMouse(true)
    local bg = panel(item, 0, 0, width, 38, {0.15, 0.18, 0.21})
    local label = text(item, title, 12, 10, width - 24, 18)
    item.clipsPerState = {DefaultState = {
        DefaultClip = function() bg:setRGB(0.15, 0.18, 0.21) end,
        Over = function() bg:setRGB(0.55, 0.27, 0.06) end
    }}
    item:registerEventHandler("mouseenter", function() bg:setRGB(0.55, 0.27, 0.06) end)
    item:registerEventHandler("mouseleave", function() bg:setRGB(0.15, 0.18, 0.21) end)
    -- LeftMouseUp delivers button_action to the element itself. Without this
    -- handler it bubbles mouse_left_click and activates the lobby's selection.
    item:registerEventHandler("button_action", function()
        if S.frontend() then action() end
        return true
    end)
    parent:addElement(item)
    item.label = label
    return item
end

local function bind(menu, controller, key, shortcut, action, label)
    menu:AddButtonCallbackFunction(menu, controller, key, shortcut, function()
        if S.frontend() then action() end
        return true
    end, function(_, owner)
        CoD.Menu.SetButtonLabel(owner, key, label)
        return true
    end, false)
end

local function registerRow()
    CoD.BO3StackRow = InheritFrom(LUI.UIElement)
    CoD.BO3StackRow.new = function(menu, controller)
        local row = LUI.UIElement.new()
        row:setClass(CoD.BO3StackRow)
        row.id = "BO3StackRow"
        row:setLeftRight(true, false, 0, 548)
        row:setTopBottom(true, false, 0, 34)
        row:makeFocusable()
        row:setHandleMouse(true)
        row:setUseStencil(true)
        local bg = panel(row, 0, 0, 548, 34, {0.95, 0.43, 0.08}, 0)
        local label = text(row, "", 10, 6, 528, 22)
        label:linkToElementModel(row, "name", true, function(model)
            label:setText(Engine.GetModelValue(model) or "")
        end)
        row.clipsPerState = {DefaultState = {
            DefaultClip = function() bg:setAlpha(0) end,
            Focus = function() bg:setAlpha(0.75) end
        }}
        return row
    end
end

local function list(menu, controller, source, x, y, count)
    local items = LUI.UIList.new(menu, controller, 3, 0, nil, false, false, 0, 0, false, true)
    -- CoD.Menu resolves keyboard/controller focus through matching child IDs
    -- and parent fields, not just through the element's native parent link.
    items.id = source
    menu[source] = items
    items:setLeftRight(true, false, x, x + 548)
    items:setTopBottom(true, false, y, y + count * 37 - 3)
    items:makeFocusable()
    items:setWidgetType(CoD.BO3StackRow)
    items:setVerticalCount(count)
    items:setDataSource(source)
    menu:addElement(items)
    return items
end

local function baseMenu(name, controller, title, subtitle)
    local menu = CoD.Menu.NewForUIEditor(name)
    menu:setOwner(controller)
    menu:setLeftRight(true, true, 0, 0)
    menu:setTopBottom(true, true, 0, 0)
    menu.soundSet = "default"
    menu.buttonModel = Engine.CreateModel(Engine.GetModelForController(controller), name .. ".buttonPrompts")
    panel(menu, 0, 0, 1280, 720, {0.025, 0.035, 0.045}, 0.98)
    panel(menu, 64, 109, 1152, 2, {0.95, 0.43, 0.08})
    text(menu, "WORKSHOP STACK  /  READY", 64, 29, 750, 16, {0.95, 0.49, 0.17})
    text(menu, title, 64, 54, 1152, 36)
    text(menu, subtitle, 64, 128, 1152, 21, {0.7, 0.75, 0.8})
    menu:registerEventHandler("gain_focus", function(self, event)
        if self.primaryList and self.primaryList:processEvent(event) then return true end
        return LUI.UIElement.gainFocus(self, event)
    end)
    LUI.OverrideFunction_CallOriginalSecond(menu, "close", function(element)
        if element.primaryList then element.primaryList:close() end
        if element.orderList then element.orderList:close() end
        Engine.UnsubscribeAndFreeModel(element.buttonModel)
    end)
    return menu
end

local function finish(menu, controller)
    menu:processEvent({name = "menu_loaded", controller = controller})
    menu:processEvent({name = "update_state", menu = menu})
    menu.primaryList:processEvent({name = "gain_focus", controller = controller})
    return menu
end

function S.editor(controller)
    if not S.frontend() then return end
    Engine.Mods_Lists_UpdateMods()
    local state = assert(S.snapshot(), "Workshop Stack is unavailable")
    local draft = S.copy(state.active)
    local focusId = state.available[1] and state.available[1].id
    local focusPane = "available"
    local busy = false
    local menu = baseMenu("MenuModsLoad", controller, "BUILD YOUR MOD STACK",
        "Select multiple mods, arrange their priority, then apply your changes together.")
    local installedTitle = text(menu, "INSTALLED MODS  /  " .. #state.available, 64, 178, 548, 20)
    local orderTitle = text(menu, "", 668, 178, 548, 20)
    panel(menu, 64, 211, 548, 337, {0.07, 0.09, 0.11})
    panel(menu, 668, 211, 548, 337, {0.07, 0.09, 0.11})
    local empty = text(menu, "No mods selected. Add mods from the left.", 682, 235, 512, 18, {0.6, 0.65, 0.7})
    local status = text(menu, "", 64, 570, 1152, 19)
    local context = text(menu, "", 64, 602, 1152, 16, {0.6, 0.65, 0.7})
    local function rows(items, available)
        local result = {}
        for _, item in ipairs(items) do
            local index = S.position(draft, item.id)
            local prefix = available and (index and "[" .. index .. "]  " or "[ + ]  ") or tostring(index) .. ".  "
            table.insert(result, {models = {id = item.id, name = prefix .. item.title}})
        end
        return result
    end
    DataSources.BO3StackAvailable = DataSourceHelpers.ListSetup("BO3StackAvailable", function() return rows(state.available, true) end, true)
    DataSources.BO3StackOrder = DataSourceHelpers.ListSetup("BO3StackOrder", function() return rows(draft, false) end, true)
    local available = list(menu, controller, "BO3StackAvailable", 64, 215, 9)
    local ordered = list(menu, controller, "BO3StackOrder", 668, 215, 9)
    menu.primaryList = available
    menu.orderList = ordered
    available.navigation = {right = ordered}
    ordered.navigation = {left = available}
    local updating = false
    for _, entry in ipairs({{available, "available"}, {ordered, "order"}}) do
        local pane = entry[2]
        entry[1]:registerEventHandler("list_item_gain_focus", function(element, event)
            if updating then return end
            focusPane = pane
            focusId = CoD.SafeGetModelValue(element:getModel(), "id")
        end)
    end
    local applyButton
    local function update()
        updating = true
        available:updateDataSource()
        ordered:updateDataSource()
        local selectedList = focusPane == "order" and ordered or available
        if #draft == 0 and focusPane == "order" then selectedList = available; focusPane = "available" end
        if focusId then selectedList:findItem({id = focusId}, nil, true, false) end
        updating = false
        orderTitle:setText("LOAD ORDER  /  " .. #draft .. " SELECTED")
        empty:setAlpha(#draft == 0 and 1 or 0)
        local dirty = S.encode(draft) ~= S.encode(state.active)
        status:setText(dirty and "Unapplied changes  -  Apply reloads the menu once you are finished." or "Your selection matches the loaded stack.")
        context:setText("1 has highest priority. Settings profile: " .. (#draft > 0 and draft[#draft].title or "none"))
        if applyButton then applyButton.label:setText(dirty and "APPLY CHANGES" or "UP TO DATE") end
    end
    local function toggle()
        if busy then return end
        local index = S.position(state.available, focusId)
        local selected = S.position(draft, focusId)
        local item = index and state.available[index] or selected and draft[selected]
        if item then
            S.toggle(draft, item)
            if focusPane == "order" and selected then
                local nextItem = draft[math.min(selected, #draft)] or state.available[1]
                focusId = nextItem and nextItem.id
            end
            update()
        end
    end
    local function move(delta)
        if not busy and S.move(draft, focusId, delta) then update() end
    end
    local function apply()
        if busy or not S.frontend() or S.encode(draft) == S.encode(state.active) then return end
        busy = true
        status:setText("Applying your mod stack...")
        if not require("bo3stack/apply/" .. S.encode(draft)) then
            busy = false
            status:setText("Could not apply. A mod may be missing. Reopen this menu and try again.")
        end
    end
    local function reset()
        if not busy then draft = S.copy(state.active); update() end
    end
    local function clear()
        if not busy then draft = {}; update() end
    end
    local function back()
        if not busy then GoBack(menu, controller) end
    end
    local function switchPane(pane)
        if busy or focusPane == pane then return end
        local items = pane == "order" and draft or state.available
        if #items == 0 then return end
        local id = S.position(items, focusId) and focusId or items[1].id
        updating = true
        available:processEvent({name = "lose_focus", controller = controller})
        ordered:processEvent({name = "lose_focus", controller = controller})
        focusPane = pane
        focusId = id
        local target = pane == "order" and ordered or available
        target:processEvent({name = "gain_focus", controller = controller})
        target:findItem({id = id}, nil, true, false)
        updating = false
    end
    applyButton = button(menu, "APPLY CHANGES", 64, 640, 190, apply)
    button(menu, "MOVE UP", 268, 640, 144, function() move(-1) end)
    button(menu, "MOVE DOWN", 426, 640, 158, function() move(1) end)
    button(menu, "RESET CHANGES", 598, 640, 190, reset)
    button(menu, "CLEAR ALL", 802, 640, 170, clear)
    button(menu, "CANCEL / BACK", 986, 640, 230, back)
    text(menu, "Enter / A: toggle    Q / LB: up    E / RB: down    F / Y: apply    R / X: reset    U / LS: clear    Esc / B: cancel", 64, 691, 1152, 16, {0.65, 0.7, 0.75})
    bind(menu, controller, Enum.LUIButton.LUI_KEY_XBA_PSCROSS, "ENTER", toggle, "Toggle mod")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_LB, "Q", function() move(-1) end, "Move up")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_RB, "E", function() move(1) end, "Move down")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_XBY_PSTRIANGLE, "F", apply, "Apply changes")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_XBX_PSSQUARE, "R", reset, "Reset changes")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_LSTICK_PRESSED, "U", clear, "Clear all")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_XBB_PSCIRCLE, nil, back, "Cancel")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_LEFT, "LEFTARROW", function() switchPane("available") end, "Installed mods")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_RIGHT, "RIGHTARROW", function() switchPane("order") end, "Load order")
    menu:subscribeToModel(Engine.CreateModel(Engine.GetGlobalModel(), "ModsGlobal.update"), function()
        local latest = S.snapshot()
        if latest and not busy then
            state.available = latest.available
            installedTitle:setText("INSTALLED MODS  /  " .. #state.available)
            update()
        end
    end, false)
    update()
    return finish(menu, controller)
end

function S.viewer(controller)
    if not S.frontend() then return end
    local state = assert(S.snapshot(), "Workshop Stack is unavailable")
    local menu = baseMenu("BO3StackLoadedMods", controller, "LOADED MODS  /  " .. #state.active,
        "Active load order. Mod 1 has the highest priority when assets overlap.")
    DataSources.BO3StackLoaded = DataSourceHelpers.ListSetup("BO3StackLoaded", function()
        local result = {}
        for index, item in ipairs(state.active) do
            table.insert(result, {models = {id = item.id, name = index .. ".  " .. item.title}})
        end
        return result
    end, true)
    panel(menu, 64, 182, 1152, 417, {0.07, 0.09, 0.11})
    menu.primaryList = list(menu, controller, "BO3StackLoaded", 84, 194, 11)
    if #state.active == 0 then text(menu, "No gameplay mods are loaded.", 84, 210, 1000, 20) end
    local back = function() GoBack(menu, controller) end
    button(menu, "CLOSE", 64, 640, 220, back)
    text(menu, "Esc / B: close    Scroll / Up / Down: browse", 308, 650, 800, 17, {0.65, 0.7, 0.75})
    bind(menu, controller, Enum.LUIButton.LUI_KEY_XBB_PSCIRCLE, nil, back, "Close")
    bind(menu, controller, Enum.LUIButton.LUI_KEY_RSTICK_PRESSED, "L", back, "Close")
    return finish(menu, controller)
end

function S.attachLobby(menu, controller)
    if not S.frontend() or menu.bo3StackPrompt then return end
    local function show()
        if S.frontend() and not menu.occludedBy then OpenPopup(menu, "BO3StackLoadedMods", controller) end
    end
    local prompt = button(menu, "", 824, 2, 392, show)
    menu.bo3StackPrompt = prompt
    local function update()
        local state = S.snapshot()
        local count = state and #state.active or 0
        local key = Engine.LastInput_Gamepad() and "RS" or "L"
        prompt.label:setText("PRESS [" .. key .. "]  LOADED MODS (" .. count .. ")")
        prompt:setAlpha(S.frontend() and 1 or 0)
    end
    bind(menu, controller, Enum.LUIButton.LUI_KEY_RSTICK_PRESSED, "L", show, "Loaded mods")
    menu:subscribeToModel(Engine.CreateModel(Engine.GetGlobalModel(), "ModsGlobal.update"), update)
    update()
end

local function checked(constructor)
    return function(controller)
        local ok, result = pcall(constructor, controller)
        if not ok then
            require("bo3stack/diagnostic/" .. tostring(result))
            error(result)
        end
        return result
    end
end
registerRow()
LUI.createMenu.BO3StackLoadedMods = checked(S.viewer)
LUI.createMenu.MenuModsLoad = checked(S.editor)
if LUI.createMenu.Lobby and LUI.createMenu.Lobby ~= S.lobbyWrapper then
    local original = LUI.createMenu.Lobby
    S.lobbyWrapper = function(controller)
        local menu = original(controller)
        S.attachLobby(menu, controller)
        return menu
    end
    LUI.createMenu.Lobby = S.lobbyWrapper
end
