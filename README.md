# AirBeam

> A solution to address audio latency when using a HomePod as the system output device on macOS over RAOP.

<img src="./resources/AirBeam.png" width="200" height="200">

[![macOS Build](https://github.com/ChenKS12138/AirBeam/actions/workflows/CI.yml/badge.svg)](https://github.com/ChenKS12138/AirBeam/actions/workflows/CI.yml)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/chenks12138)

## 🧪 Compare with Native Speaker & AirPlay

🖥️ Test Environment

* **Laptop**: MacBook Pro 14-inch (2021)
  * **OS**: macOS Sequoia 15.4
  * **Network**: Wired Ethernet connection
* **Speaker**: HomePod mini (2020)
  * **OS**: 18.5
  * **Network**: Connected via 5GHz Wi-Fi

> **⚠️ PLEASE TURN OFF MUTE BEFORE PLAYING THE VIDEOS!**

| Native Speaker  | Airplay | AirBeam |
| ------------- | ------------- | ------------- |
| <video src="https://github.com/user-attachments/assets/2b89ad33-e055-4cbe-95da-fc29a9156109.mp4">  | <video src="https://github.com/user-attachments/assets/40815cc9-0c97-4dd4-8b64-6e78be526605.mp4">| <video src="https://github.com/user-attachments/assets/defb753d-1c00-47cb-9ab6-cec83c31e919.mp4">|


##  📦 Installation

There are several ways:

* ⬇️ Download [the latest release on Github](https://github.com/ChenKS12138/AirBeam/releases)
* 🚀 Clone it and build it yourself

## ⚙️ Usage

1. Launch the app  
2. Click **Install**  
3. Select your output device in the macOS audio control panel  

<img width="748" alt="image" src="https://github.com/user-attachments/assets/24f7904b-7238-4815-89bb-3fc4519b4269" />


## 🛠️ Build from Source

```shell
git clone git@github.com:ChenKS12138/AirBeam.git
cd AirBeam
cmake -S . -B build
# or try `cmake -S . -B build -GNinja`, I prefer to use Ninja, it's faster.
cmake --build build --target AirBeam
open build/source/AirBeam/AirBeam.app
```

## 🪛 Troubleshooting

```shell
git clone git@github.com:ChenKS12138/AirBeam.git
cd AirBeam
bash ./run_airbeam_doctor.sh

# This script will search for RAOP services on the network, you need to enter the number to select one, the script will transmit audio to the RAOP service, and print debug logs to the `mylog.log` file.
# For feedback, please provide the `mylog.log` file.
```

<img width="932" alt="image" src="https://github.com/user-attachments/assets/73ae18b9-c6f1-4437-95fb-0584353d2120" />


## 🙏 Credits

Parts of this project were inspired by or adapted from the following open source projects:

- [gavv/libASPL](https://github.com/gavv/libASPL) – for creating macOS Audio Server plugins.
- [LinusU/rust-roap-player](https://github.com/LinusU/rust-raop-player) – reference for AirPlay integration.

Huge thanks to their authors and contributors! 💖

## 🤝 Contributing

Contributions are welcome! Feel free to open an issue or submit a PR.
