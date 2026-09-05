[中文](README-zh.md)

# OpenMinecraft

New engine/client project

## Submodules

- openminecraft-core/binary (string hash, endians)
- openminecraft-core/geom (font shaping wrapper, fallbacks)
- openminecraft-core/i18n (internationalization utils)
- openminecraft-core/io (json parser)
- openminecraft-core/log (Logging system)
- openminecraft-core/mem (memory tracker & allocator)
- openminecraft-core/network (network transfer wrappers, WIP)
- openminecraft-core/renderer (renderer api backend abstraction layer)
- openminecraft-core/specs (parsers for jpeg, png, zip etc.)
- openminecraft-core/util (internal utils)
- openminecraft-core/vfs (virtual file system)
- openminecraft-core/vm (elysia vm implementation)
- openminecraft-core/world (in-game data management)
- openminecraft (shell layer & demo)

## How to build

### Requirements

xmake<br>
python<br>
cmake, meson, ninja (optional)

### Steps

#### Download Resources

Run ```python scripts/fetchsrc.py```

#### Pack Bundles

##### Windows

Ensure you have the ```zip``` utility installed<br>
Use ```choco install zip``` to install<br>
Use the following command to create the required bundles (powershell)

```
cd bootassets
zip -9 -r boot.bundle .
mv boot.bundle ..
cd ..
cd externalassets
zip -9 -r external.bundle .
mv external.bundle ..
cd ..
```

Or you can create the two bundles manually (folder ```boot``` and ```externalassets```, no top-level folders), and save them to ```boot.bundle``` and ```external.bundle```

##### Otherwise

run ```sh scripts/updateassets.sh```

#### Build

run ```xmake``` and wait

#### Demo

run ```xmake run openminecraft [gl/vk]``` to switch the renderer
