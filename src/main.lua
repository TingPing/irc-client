local lgi = require('lgi')
local Gio = lgi.require('Gio')
local IrcGui = lgi.package('IrcGui') -- Annoying split for now..

package.loaders[#package.loaders + 1] = function (str)
  local path = '/se/tingping/IrcClient/' .. str .. '.lua'
  local data = Gio.resources_lookup_data(path, Gio.ResourceLookupFlags.NONE)
  if data then
    local mod, err = loadstring(data:get_data() or '', str)
    if not mod then
      return err
    end
    return mod
  end
end

require('application')
local app = IrcGui.Application {
  application_id = 'se.tingping.IrcClient',
  flags = Gio.ApplicationFlags.HANDLES_OPEN
}
return app:run(arg)
