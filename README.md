# Server-side Auto Bunnyhop for Left 4 Dead 2
This plugin works both on server and client (to remove shaky and laggy effect when you bunnyhop) side, but it requires a server (listen or dedicated whatever) to run this plugin, otherwise auto bunnyhop won't work.

Note: this is Valve Server Plugin which is safely use, it doesn't require any injectors or some other magic.

# Installation
First, download the DLL from Releases and move it to the folder `Left 4 Dead 2/left4dead2/`, then launch the game with launch parameter `-insecure`.

Type the following command in the console: `plugin_load autobhop`.

The game will load the plugin and you will see green message about successful load (otherwise, red).

### Auto Load
To do not bother with manual loading you can tell the game to load the plugin automatically.

Go to this folder `Left 4 Dead 2/left4dead2/addons/` and create file with such name `autobhop.vdf`.

Then, copy and paste the following text to the file and save it.

```
"Plugin"
{
	"file"	"autobhop"
}
```

Now launch the game (with launch parameter `-insecure` ofc), the plugin will be loaded automatically.

# Server Part
The server part of plugin provides the bunnyhop and controls what client can use it.

To disable or enable auto bunnyhop for everyone, use the following console variable (enabled by default): `sv_autobunnyhop <0/1>`

# Client Part
The client part of plugin uses a bridge (user messages and client commands) with server to make it possible to toggle auto bunnyhop.

**The most important feature: if you will play on server that has this plugin, you won't see the shaky and laggy effect during bunnyhopping**.

To disable or enable auto bunnyhop for yourself, use the following console command (console won't hint it because it's server-side): `cl_autobunnyhop` (you can also pass additional argument `1` or `0` to force auto bunnyhop be enabled/disabled, i.e. `cl_autobunnyhop 1`)
