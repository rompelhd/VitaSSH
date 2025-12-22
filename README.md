# VitaSSH – Interactive SSH Client for PS Vita by Rompelhd

VitaSSH is a streamlined SSH client designed specifically for the PlayStation Vita, leveraging **libssh2** for secure shell connections and **VitaSDK** for native Vita hardware integration. This client provides **interactive SSH terminal sessions** on the Vita's unique platform.

## 🛠 Technical Foundation

**Built with:**
- **Libssh2**: Comprehensive SSH2 protocol implementation.
- **Vita2d**: Vita2d library for creating 2D graphics.
- **VitaSDK**: Official PlayStation Vita homebrew SDK.
- **IME Dialog System**: Vita's on-screen keyboard interface.

## ⚡ Features

### ✅ Working Features
- Secure SSH2 connections with password authentication.
- **Interactive shell sessions** with full terminal emulation.
- PTY (pseudo-terminal) support for interactive programs.
- ANSI escape sequence processing for colored output.
- Special key support (Ctrl+C, Tab, Arrow keys, etc.).
- Real-time command input and output.
- Memory-safe resource cleanup.

### ⚠️ Current Limitations
- Password authentication only (no key-based authentication yet).
- Limited character set: ASCII-only display output.
- Experimental status: Not for production use.

## 🔄 Technical Challenges Solved
- **Vita IME Integration**: Bridging UTF-16 keyboard to ASCII SSH.
- **Graphics Context Switching**: Seamless GXM ↔ debugScreen transitions.
- **Memory Management**: CDRAM/User RAM allocation strategies.
- **Network Reliability**: Vita-specific NetCtl state handling.
- **Output Sanitization**: ANSI/Unicode filtering for text display.
- **Interactive Terminal Emulation**: Full PTY support with xterm compatibility.

## 🚀 Build & Deployment

```bash
# VitaSDK environment required
mkdir build && cd build
cmake ..
make
```
## 📦 Generates

- `ssh_client_vita.self` (Vita executable)  
- `ssh_client_vita.vpk` (Installation package)

---

## 🎯 Use Cases

- Remote server administration from the PS Vita with full interactive shell
- Running interactive programs like `vim`, `nano`, `htop`, `top`
- Real-time system monitoring and management
- Educational demonstration of Vita homebrew capabilities
- SSH protocol testing in constrained environments

---

## 📋 Prerequisites

- PlayStation Vita with homebrew enabled
- VitaSDK development environment
- `libssh2` library compiled for Vita
- OpenSSL libraries for Vita

---

## 🔧 Installation

1. Copy the generated `.vpk` file to your Vita.
2. Install it using VitaShell or the Package Manager.
3. Make sure you have network connectivity (WiFi required).
4. Launch VitaSSH and configure your connection details.

---

## 🎮 Controls

### Main Screen
- `TRIANGLE`: Configure SSH credentials and connect
- `START`: Exit the application

### Interactive Shell Mode
- `TRIANGLE`: Open on-screen keyboard for command input
- `SQUARE`: Open special keys menu (Ctrl+C, Tab, Arrow keys, etc.)
- `X`: Send Ctrl+C signal
- `CIRCLE`: Send Tab
- `L Trigger`: Arrow Up
- `R Trigger`: Arrow Down
- `SELECT`: Resize terminal
- `START`: Exit shell and disconnect

---

## ⚠️ Important Notes

- This is **experimental software** — use at your own risk.
- Some SSH servers may require specific configuration.
- Password authentication only (no key-based authentication yet).

---

## 🐛 Known Issues

- Character encoding limitations for non-ASCII text
- Occasional timeouts on unstable network connections

---

## 🤝 Contributing

This project is open to contributions! Areas for improvement:

- SSH key authentication
- UTF-8 character support
- Copy/paste functionality
- Session management  

---

> **Simplified • Interactive Shell Only • Vita Homebrew**  
> *Powered by libssh2 1.11.0+ and VitaSDK toolchain for native Vita SSH interactive terminal capabilities.*

**Disclaimer:** This software is provided *as-is* for educational and experimental purposes. Use responsibly and in compliance with applicable laws and terms of service.
