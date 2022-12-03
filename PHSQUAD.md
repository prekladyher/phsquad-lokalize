# PHSquad's Lokalize

## Development Environment

* Use craft to setup development and build environment (`craft lokalize`).
* Set Visual Studio's cmake executable to the one used by Craft.

Example CMakeSettings.json:

```json
{
  "configurations": [
    {
      "name": "x64-Debug",
      "generator": "Ninja",
      "configurationType": "Debug",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "buildRoot": "${projectDir}\\out\\build\\${name}",
      "installRoot": "${projectDir}\\out\\install\\${name}",
      "cmakeCommandArgs": "",
      "buildCommandArgs": "",
      "ctestCommandArgs": "",
      "cmakeExecutable": "O:/build/craft/dev-utils/bin/cmake.exe"
    }
  ]
}
```

Use fork repo for the blueprint by adding `etc/blueprints/locations/craft-blueprints-kde/kde/kdesdk/lokalize/version.ini`
with the following content:

```ini
[General]
name = Lokalize
branches = phsquad
tarballs =
defaulttarget = phsquad
gitUrl = https://github.com/prekladyher/phsquad-lokalize.git
```

## Creating Installer

Installer is created by Craft. Update `etc/BlueprintSettings.ini` with the following:

```ini
[kde/kdesdk/lokalize]
branch = master
```
