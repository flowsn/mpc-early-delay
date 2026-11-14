# Build

Requires Python 3.9+. The SDK is included as a git submodule at `sdk/` (public, MIT).

```
git clone --recurse-submodules <this repo>
python3 sdk/tools/build.py plugin        # once plugin/ exists
```
First build downloads the Zig cross compiler (~50 MB). Output: `plugin/build/<Manufacturer> - VST - <Name>/`, copy into `VST2 Plugins/` on the MPC drive or install with `sdk/tools/mpc_remote.py <MPC-IP> add <folder>` (MPC developer mode on, same network). MPC OS 3.10.1+ with External Plugin Support enabled.
