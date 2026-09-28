"""Run the production front-end Lua against a small BO3 UI/engine fixture.

Requires lupa with Lua 5.1; independent of the legacy launcher tests.
"""
from pathlib import Path
import re
from lupa.lua51 import LuaRuntime

source = (Path(__file__).resolve().parents[1] / 'workshop/ui/bo3stack/frontend.lua').read_text()
embedded = Path(__file__).resolve().parents[1] / 'native/build/generated/frontend_source.hpp'
if embedded.exists():
    assert ''.join(re.findall(r'R"stackui\((.*?)\)stackui"', embedded.read_text(), re.S)) == source, 'Embedded UI differs from production Lua'
lua = LuaRuntime()
lua.execute(r'''
map = 'core_frontend'
calls = {}
active = {{id='a', title='Alpha'}, {id='b', title='Beta'}}
available = {{id='a', title='Alpha'}, {id='b', title='Beta'}, {id='c', title='Charlie'}}
Engine = {
    GetCurrentMap=function() return map end,
    Mods_Lists_UpdateMods=function() end,
    GetGlobalModel=function() return {} end,
    GetModelForController=function() return {} end,
    CreateModel=function() return {} end,
    GetModelValue=function(model) return model.value end,
    UnsubscribeAndFreeModel=function() end,
    LastInput_Gamepad=function() return false end
}
function require(request)
    if request == 'bo3stack/state' then
        BO3StackNativeState = {active=active, available=available}
        return true
    end
    table.insert(calls, request)
    return not rejectApply
end
Enum = {LUIAlignment={LUI_ALIGNMENT_LEFT=0}, LUIButton={}}
setmetatable(Enum.LUIButton, {__index=function(_, key) return key end})
DataSources = {}
DataSourceHelpers = {ListSetup=function(_, fn) return fn end}
local methods = {}
for _, name in ipairs({'setLeftRight','setTopBottom','setTTF','setAlignment','setRGB','setHandleMouse',
    'makeFocusable','setOwner','setClass','setUseStencil','setWidgetType','setVerticalCount','linkToElementModel','close'}) do
    methods[name]=function() end
end
function methods:setText(value) self.text = value end
function methods:setAlpha(value) self.alpha = value end
function methods:setDataSource(source) self.source=source; self:updateDataSource() end
function methods:updateDataSource() self.rows=DataSources[self.source]() end
function methods:findItem(model) self.selected=model.id end
function methods:registerEventHandler(event, fn) self.events[event]=fn end
function methods:addElement(element) table.insert(self.children, element) end
function methods:AddButtonCallbackFunction(_, _, key, _, fn) self.keys[key]=fn end
function methods:processEvent() end
function methods:subscribeToModel(_, fn, immediate) if immediate ~= false then fn({}) end end
function element() return setmetatable({children={}, events={}, keys={}}, {__index=methods}) end
LUI = {createMenu={}, UIElement={new=element}, UIImage={new=element}, UIText={new=element}, UIList={new=element},
    OverrideFunction_CallOriginalSecond=function() end}
CoD = {Menu={NewForUIEditor=element, SetButtonLabel=function() end}, SafeGetModelValue=function(model,key) return model[key] end}
function InheritFrom() return {} end
function GoBack() closed=true end
function OpenPopup(_,name) opened=name end
''')
lua.execute(source)
lua.execute(r'''
local S = BO3StackUI
local draft = S.copy(active)
S.toggle(draft, available[3])
S.toggle(draft, available[2])
assert(S.encode(active)=='a,b' and S.encode(draft)=='a,c')
assert(S.move(draft,'c',-1) and S.encode(draft)=='c,a')
assert(not S.move(draft,'c',-1) and not S.move(draft,'a',1))
assert(not S.move(draft,'missing',1))
local menu = S.editor(0)
local function clickButton(owner, title)
    for _, child in ipairs(owner.children) do
        if child.label and child.label.text == title then
            assert(child.events.button_action, 'LUI LeftMouseUp would bubble into the parent action')
            child.events.button_action()
            return
        end
    end
    error('Missing button: '..title)
end
assert(menu[menu.primaryList.id]==menu.primaryList and menu[menu.orderList.id]==menu.orderList,
    'CoD.Menu cannot resolve the child IDs used by keyboard/controller focus')
local keys = Enum.LUIButton
menu.primaryList.events.list_item_gain_focus({getModel=function() return {id='c'} end}, {})
menu.keys[keys.LUI_KEY_XBA_PSCROSS]()
assert(#calls==0 and #menu.orderList.rows==3, 'Selection triggered a reload')
menu.keys[keys.LUI_KEY_LB]()
assert(#calls==0 and menu.orderList.rows[2].models.id=='c', 'Reordering reloaded or lost focus')
menu.keys[keys.LUI_KEY_RIGHT]()
assert(menu.orderList.selected=='c', 'Switching panes lost the selected mod')
menu.keys[keys.LUI_KEY_LEFT]()
assert(menu.primaryList.selected=='c', 'Returning to installed pane lost focus')
clickButton(menu, 'APPLY CHANGES')
menu.keys[keys.LUI_KEY_XBY_PSTRIANGLE]()
assert(#calls==1 and calls[1]=='bo3stack/apply/a,c,b', 'Apply not batched or double-fired')
assert(S.encode(active)=='a,b', 'Fixture applied state changed while editing')
calls={}
menu=S.editor(0)
menu.keys[keys.LUI_KEY_XBY_PSTRIANGLE]()
assert(#calls==0, 'Unchanged Apply reloaded')
menu.keys[keys.LUI_KEY_XBA_PSCROSS]()
menu.keys[keys.LUI_KEY_XBX_PSSQUARE]()
assert(#menu.orderList.rows==2 and #calls==0, 'Reset must restore applied order without reloading')
menu.keys[keys.LUI_KEY_LSTICK_PRESSED]()
assert(#menu.orderList.rows==0 and #calls==0, 'Clear all must be staged until Apply')
menu.keys[keys.LUI_KEY_XBX_PSSQUARE]()
menu.keys[keys.LUI_KEY_XBB_PSCIRCLE]()
assert(closed and #calls==0, 'Cancel reloaded')
local viewer=S.viewer(0)
assert(#viewer.primaryList.rows==2 and viewer.primaryList.rows[1].models.id=='a')
assert(viewer[viewer.primaryList.id]==viewer.primaryList, 'Viewer list cannot receive directional input')
assert(not viewer.keys[keys.LUI_KEY_XBA_PSCROSS], 'Loaded viewer must be read-only')
closed=false
clickButton(viewer, 'CLOSE')
assert(closed, 'Viewer mouse close failed')
local lobby=element()
S.attachLobby(lobby,0)
assert(lobby.bo3StackPrompt.events.button_action, 'Prompt click bubbles into the lobby selection')
lobby.bo3StackPrompt.events.button_action()
assert(opened=='BO3StackLoadedMods', 'Prompt did not open the loaded-mod viewer')
rejectApply=true
menu=S.editor(0)
menu.keys[keys.LUI_KEY_XBA_PSCROSS]()
menu.keys[keys.LUI_KEY_XBY_PSTRIANGLE]()
closed=false
menu.keys[keys.LUI_KEY_XBB_PSCIRCLE]()
assert(closed, 'Failed Apply locked navigation')
map='zm_prototype'
local count=#calls
assert(S.snapshot()==nil and S.editor(0)==nil and S.viewer(0)==nil)
menu.keys[keys.LUI_KEY_XBY_PSTRIANGLE]()
assert(#calls==count, 'UI callback modified a running match')
''')
# Executing the installer inside a match must not register any UI at all.
game = LuaRuntime()
game.execute("Engine={GetCurrentMap=function() return 'zm_prototype' end}")
game.execute(source)
assert game.globals().BO3StackUI is None
print('Lua 5.1: batch edits, reorder bounds, focus identity, apply once, no-op apply, reset/cancel, failure recovery, active viewer and gameplay exclusion passed.')
