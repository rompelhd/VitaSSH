# VitaSSH – SSH Client for PS Vita by Rompelhd

VitaSSH is a proof-of-concept SSH client designed specifically for the PlayStation Vita, leveraging **libssh2** for secure shell connections and **VitaSDK** for native Vita hardware integration. This experimental client provides basic SSH terminal functionality on the Vita's unique platform.

## 🛠 Technical Foundation

**Built with:**
- **libssh2**: Comprehensive SSH2 protocol implementation.
- **VitaSDK**: Official PlayStation Vita homebrew SDK.
- **PSVita GXM**: Low-level graphics framework for Vita.
- **IME Dialog System**: Vita's on-screen keyboard interface.

## 📁 Project Structure

```plaintext
vitassh2/
├── CMakeLists.txt          # VitaSDK build configuration
├── include/               # Header files
│   ├── graphics.h         # GXM display management
│   ├── network.h          # Vita network stack wrapper
│   ├── ssh_client.h       # libssh2 session handling
│   ├── input.h            # IME dialog interfaces
│   └── debugScreen.h      # Text output system
└── src/                   # Source implementation
    ├── main.c             # Application lifecycle & state machine
    ├── graphics.c         # Double-buffered rendering
    ├── network.c          # NetCtl & socket initialization
    ├── ssh_client.c       # SSH command execution engine
    ├── input.c            # Keyboard input handling
    └── debugScreen.c      # VRAM text rendering
```
## 🔧 Core Architecture

### Graphics Subsystem (`graphics.c/h`)
- GXM double-buffered rendering for a smooth UI.
- CDRAM memory allocation for GPU-optimized buffers.
- VSync-synchronized swapping to prevent tearing.
- Automatic display queue management.

### Network Layer (`network.c/h`)
- 512KB dedicated network memory block allocation.
- SCE_NETCTL state monitoring for connection tracking.
- Modular system module loading (SceNet, SceNetCtl).
- 10-second connection timeout with status feedback.

### SSH Engine (`ssh_client.c/h`)
- libssh2 session lifecycle management.
- Character filtering system for Vita-safe output.
- ANSI escape sequence stripping for compatibility.
- Non-blocking channel execution with error handling.

### Input System (`input.c/h`)
- Sequential credential collection (IP → Port → User → Password).
- UTF-16 to ASCII conversion for Vita IME compatibility.
- Modal dialog state management.
- Password field security with hidden input.

## 🎮 User Interface Flow

### Configuration Phase (GXM Graphics)
- IP Address input.
- SSH Port selection (default: 22).
- Username entry.
- Password input (masked).

### Connection Phase (Debug Screen)
- Network module initialization.
- SSH handshake and authentication.
- Status reporting.

### Command Phase (Hybrid Graphics/Debug)
- **TRIANGLE**: Open IME keyboard for commands.
- **SQUARE**: Exit application.
- Dynamic graphics reinitialization for input dialogs.

## ⚡ Features and Limitations

### ✅ Working Features
- Secure SSH2 connections with password authentication.
- Arbitrary command execution on remote systems.
- Special character filtering for Vita display compatibility.
- Network state monitoring with visual feedback.
- Memory-safe resource cleanup.

### ⚠️ Current Limitations
- No scrollback: Screen clears after each command.
- No command history: Manual retyping required.
- No touch input: Physical buttons only (TRIANGLE/SQUARE).
- Limited character set: ASCII-only display output.
- No interactive sessions: Single commands only.
- Experimental status: Not for production use.

## 🔄 Technical Challenges Solved
- **Vita IME Integration**: Bridging UTF-16 keyboard to ASCII SSH.
- **Graphics Context Switching**: Seamless GXM ↔ debugScreen transitions.
- **Memory Management**: CDRAM/User RAM allocation strategies.
- **Network Reliability**: Vita-specific NetCtl state handling.
- **Output Sanitization**: ANSI/Unicode filtering for text display.

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

- Remote server administration from the PS Vita  
- Quick file system inspection using `ls`, `df`, etc.  
- Network troubleshooting with `ping`, `netstat`  
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

- `TRIANGLE`: Open on-screen keyboard for commands  
- `SQUARE`: Exit the application  
- `D-Pad/Stick`: Navigate through IME dialogs  
- `X Button`: Confirm/enter in dialogs

---

## ⚠️ Important Notes

- This is **experimental software** — use at your own risk.  
- Some SSH servers may require specific configuration.  
- Complex commands with extensive output may cause display issues.  
- Password authentication only (no key-based authentication yet).

---

## 🐛 Known Issues

- Limited output buffer for long command responses  
- No support for interactive programs (`vim`, `top`, etc.)  
- Character encoding limitations for non-ASCII text  
- Occasional timeouts on unstable network connections

---

## 🤝 Contributing

This project is open to contributions! Areas for improvement:

- Scrollback functionality  
- Command history  
- Touchscreen integration  
- SSH key authentication  

---

> **Experimental • Proof of Concept • Vita Homebrew**  
> *Powered by libssh2 1.11.0+ and VitaSDK toolchain for native Vita SSH capabilities.*

**Disclaimer:** This software is provided *as-is* for educational and experimental purposes. Use responsibly and in compliance with applicable laws and terms of service.
