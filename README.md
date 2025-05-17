# AirBeam

> A solution to address audio latency when using a HomePod as the system output device on macOS over RAOP.

<img src="./images/AirBeam.png" width="200" height="200">

[![macOS Build](https://github.com/ChenKS12138/AirBeam/actions/workflows/CI.yml/badge.svg)](https://github.com/ChenKS12138/AirBeam/actions/workflows/CI.yml)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/chenks12138)

## Compare With NativeSpeaker & Airplay

**PLEASE TURN OFF MUTE**

| Native Speaker  | Airplay | AirBeam |
| ------------- | ------------- | ------------- |
| <video src="https://github.com/user-attachments/assets/2b89ad33-e055-4cbe-95da-fc29a9156109.mp4">  | <video src="https://github.com/user-attachments/assets/40815cc9-0c97-4dd4-8b64-6e78be526605.mp4">| <video src="https://github.com/user-attachments/assets/defb753d-1c00-47cb-9ab6-cec83c31e919.mp4">|


## Installation

There are several ways:

* ⬇️ Download [the latest release on Github](https://github.com/ChenKS12138/AirBeam/releases)
* 🚀 Clone it and build it yourself

## Usage

Just Click "Install", and choose device in your audio control panel. 

<img width="748" alt="image" src="https://github.com/user-attachments/assets/24f7904b-7238-4815-89bb-3fc4519b4269" />


## Build from resource

```shell
git clone git@github.com:ChenKS12138/AirBeam.git
cd AirBeam
cmake -S . -B build
# or try `cmake -S . -B build -GNinja`, I prefer to use Ninja, it's faster.
cmake --build build --target AirBeam
open build/source/AirBeam/AirBeam.app
```
