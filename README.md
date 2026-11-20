# RÜZGAR AI
### A Cross-Provider AI Desktop Client

**One Interface. Multiple AI Providers. Two Editions.**

RÜZGAR AI is a cross-provider AI desktop application designed to bring multiple AI services and local models together in one customizable interface.

The project provides two editions:

- **RÜZGAR AI C++ Edition** — Built with C++17 and Qt 6.
- **RÜZGAR AI Python Edition** — A Python implementation featuring the same functionality as the C++ Edition.

Both editions are designed to provide the same user experience and feature set. **Windows executable (`.exe`) releases are available for both editions**, allowing users to run the application without manually setting up the development environment.

---

## Features

Both C++ and Python Editions include the same features:

- **Multiple AI Providers:** Connect to OpenAI, Anthropic, Google Gemini, DeepSeek and other compatible providers.
- **Local AI Connectivity:** Connect to local inference servers such as LM Studio and vLLM.
- **Custom API Endpoints:** Configure custom endpoints and OpenAI-compatible APIs.
- **Streaming Responses:** Receive AI responses in real time.
- **Model Management:** Configure providers, API keys, endpoints and models.
- **Model Discovery:** Retrieve available models from supported providers.
- **Conversation Management:** Create, rename, search, pin and delete conversations.
- **Persistent Chat History:** Keep conversations saved locally.
- **Project Organization:** Group conversations into projects.
- **Markdown Support:** View formatted responses and export conversations.
- **Code Blocks:** Syntax-highlighted code blocks with copy functionality.
- **Image Attachments:** Send images to supported vision models.
- **Response Controls:** Regenerate responses and provide feedback.
- **Personal Instructions:** Customize AI behavior with personal instructions.
- **User Profiles:** Manage local user profiles.
- **Themes:** Light, dark and system appearance options.
- **Hardware Monitoring:** Monitor available hardware information.
- **Local Database:** Store application data using SQLite.

## Two Editions, Same Features

| Feature | C++ Edition | Python Edition |
|---|---|---|
| Complete feature set | Yes | Yes |
| AI provider integrations | Yes | Yes |
| Local model connectivity | Yes | Yes |
| Streaming responses | Yes | Yes |
| Conversation management | Yes | Yes |
| Projects and chat history | Yes | Yes |
| Image attachments | Yes | Yes |
| Themes and customization | Yes | Yes |
| Windows `.exe` Release | Yes | Yes |

The two editions are separate implementations of the same RÜZGAR AI application. Choosing either edition does not mean giving up application features.

## Supported Providers

RÜZGAR AI supports connections to:

- OpenAI
- Anthropic
- Google Gemini
- DeepSeek
- OpenAI-compatible API providers
- Local inference servers
- Custom API endpoints

Provider availability depends on the configured endpoint, model and API compatibility.

## Technology Stack

### C++ Edition

| Technology | Purpose |
|---|---|
| C++17 | Core application |
| Qt 6 | Desktop interface |
| Qt Widgets | User interface components |
| Qt Network | API communication |
| Qt SQL | Database integration |
| SQLite | Local data storage |
| Windows Credential Manager | Credential storage on supported Windows builds |

### Python Edition

The Python Edition provides the same application functionality through a separate Python implementation.

Its implementation-specific dependencies and technical details will be documented separately.

## Windows Releases

RÜZGAR AI provides executable releases for both editions.

- **C++ Edition:** Windows `.exe` release.
- **Python Edition:** Windows `.exe` release.

Users can choose their preferred edition and run its executable without needing to build the application from source.

Check the repository's **Releases** section for available downloads.

## Building from Source

### C++ Edition

The C++ Edition uses C++17 and Qt 6.

Requirements:

- Qt 6.2 or newer
- C++17-compatible compiler
- Qt Core
- Qt Gui
- Qt Widgets
- Qt Network
- Qt Sql

Visual Studio 2022 with the MSVC toolchain is recommended for Windows development.

The current C++ implementation is maintained in a source file. Build instructions may evolve as the project structure develops.

### Python Edition

The Python Edition can also be built and packaged independently. Its Windows executable release is intended to make installation easier for users who do not want to configure a Python development environment.

## Project Structure

The following is the planned repository organization:

```text
RuzgarAI/
│
├── C++ Version/
│   ├── main.cpp
│   └── README.md
│
├── PYTHON Version/
│   ├── main.py
│   └── README.md
│
├── assets/
├── docs/
├── LICENSE
└── README.md
```

## Development Status

RÜZGAR AI is being developed as a multi-edition desktop AI client.

Current project goals:

- Maintain feature parity between C++ and Python Editions.
- Provide Windows executable releases for both editions.
- Support multiple AI providers through one interface.
- Improve local model connectivity and compatibility.
- Continue improving the desktop experience.

Some interface sections and planned features may still be under development.

## License

License information will be added to the repository.

---

**RÜZGAR AI — One Interface. Multiple AI Providers. Two Editions.**
