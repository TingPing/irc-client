local lgi = require('lgi')
local GLib = lgi.require('GLib')
local GObject = lgi.require('GObject')
local Gio = lgi.require('Gio')
local Gtk = lgi.require('Gtk', '3.0')
local GIRepository = lgi.require('GIRepository', '2.0')
local Peas = lgi.require('Peas', '1.0')
local IrcClient = lgi.require('IrcClient', '0.1')
local IrcGui = lgi.IrcGui
local log = lgi.log.domain('IrcClient')


local Application = IrcGui:class('Application', Gtk.Application)

Application._property.css_provider = GObject.ParamSpecObject(
  'css-provider', 'CSS Provider', '', Gtk.CssProvider, { 'READWRITE' }
)

Application._property.settings = GObject.ParamSpecObject(
  'settings', 'Settings', '', Gio.Settings, { 'READWRITE' }
)

Application._property.extensions = GObject.ParamSpecObject(
  'extensions', 'Extensions', '', Peas.ExtensionSet, { 'READWRITE' }
)

function Application:_init()
  self.css_provider = Gtk.CssProvider()
  self.settings = Gio.Settings.new('se.tingping.IrcClient')
end

-- Take "Sans Foo 12" and turn it into CSS
-- TODO: Hand more edge cases like weights
local function css_from_pango_description(desc)
	local i, j = desc:find('%s%d+$')
	if i then
		local family = desc:sub(0, i - 1)
		local size = desc:sub(i + 1, j)
		return string.format(
			'.irc-textview { font-family: %s; font-size: %spx }', family, size)
	else
		return string.format('.irc-textview { font-family: %s }', desc)
	end
end

local function font_changed(app)
	local font = app.settings:get_string('font')
	local css = css_from_pango_description(font)
	local status, err = app.css_provider:load_from_data(css)
	if not status then
		log.warning('Failed loading font css: ' .. err)
	end
end

function Application:do_constructed()
  self._type._parent.do_constructed(self)
  self:add_option_group(GIRepository.Repository.get_option_group())
  self.settings.on_changed['font'] = function(pspec) font_changed(self) end
  font_changed(self) -- Set it now also
end

local function load_plugins(app)
  local engine = Peas.Engine.get_default()
  local plugin_path = GLib.build_filenamev({config.LIBDIR, 'irc-client'})
  local user_plugin_path = GLib.build_filenamev({GLib.get_home_dir(), ".local", "lib", "irc-client"})
  engine:add_search_path(plugin_path)
  engine:add_search_path(user_plugin_path)

  app.extensions = Peas.ExtensionSet.new(engine, Peas.Activatable, {object=app})
  app.settings:bind('enabled-plugins', engine, 'loaded-plugins', Gio.SettingsBindFlags.DEFAULT)
  local on_extension_added = function(set, info, extension) extension:activate() end
  local on_extension_removed = function(set, info, extension) extension:deactivate() end
  app.extensions:foreach(on_extension_added)
  app.extensions.on_extension_added = on_extension_added
  -- FIXME! app.extensions.on_extension_removed = on_extension_removed
end

function Application:do_startup()
  self._type._parent.do_startup(self)

  self:add_action(Gio.SimpleAction {
    name = 'quit',
    on_activate = function (action, param)
      self:quit()
    end
  })

  self:add_action(Gio.SimpleAction {
    name = 'about',
    on_activate = function (action, param)
      local dialog = Gtk.AboutDialog {
        transient_for = self.active_window,
        program_name = 'IrcClient',
        version = config.PACKAGE_VERSION,
        license_type = Gtk.License.GPL_3_0
      }
      dialog:present()
    end
  })

  self:add_action(Gio.SimpleAction {
    name = 'preferences',
    on_activate = function (action, param)
      local dialog = IrcClient.PreferencesWindow {
        transient_for = self.active_window,
        modal = true,
      }
      dialog:present()
    end
  })

  self:add_action(Gio.SimpleAction {
    name = 'focus-context',
    parameter_type = GLib.VariantType('s'),
    on_activate = function (action, param)
      local actions = self.active_window:get_action_group()
      actions:activate_action('focus', param)
    end
  })
end

function Application:do_activate()
  local win = self.active_window
  if not win then
    win = IrcClient.Window({application=self})
	Gtk.StyleContext.add_provider_for_screen(win:get_screen(),
      self.css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
    load_plugins(self) -- Done here for plugins that require the UI
  end
  win:present()
end

function Application:do_open(files, hint)
  self:activate()

  for i, file in ipairs(files) do
    local scheme = file:get_uri_scheme()
    if scheme ~= 'irc' and scheme ~= 'ircs' then
      log.warning('Recieved invalid uri!')
    else
      -- TODO: Implement opening irc urls
      log.warning('Opening URLs not yet implemented')
    end
  end
end
