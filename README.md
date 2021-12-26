# Homebrew launcher wrapper payload
A simple wrapper that is just executing [homebrew_launcher_installer](https://github.com/wiiu-env/homebrew_launcher_installer).

## Usage
Place the `50_hbl_installer.rpx` in the `[ENVIROMENT]/modules/setup` folder and run the [EnvironmentLoader](https://github.com/wiiu-env/EnvironmentLoader).
Requires [MochaPayload](https://github.com/wiiu-env/MochaPayload) as a setup module in `[ENVIROMENT]/modules/setup` when coldbooting.
Read the intructions in the [original repo](https://github.com/wiiu-env/homebrew_launcher_installer) for further usage. 
TLDR: Put the [homebrew_launcher.elf](https://github.com/dimok789/homebrew_launcher) in `sd:/wiiu/apps/homebrew_launcher/homebrew_launcher.elf`. When coldbooting you need to open the Mii Maker to start it.

## Building

For building you just need [wut](https://github.com/devkitPro/wut/) installed, then use the `make` command.

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t hblinstallerwrapper-builder

# make 
docker run -it --rm -v ${PWD}:/project hblinstallerwrapper-builder make

# make clean
docker run -it --rm -v ${PWD}:/project hblinstallerwrapper-builder make clean
```

## Credits
See [homebrew_launcher_installer](https://github.com/wiiu-env/homebrew_launcher_installer) 