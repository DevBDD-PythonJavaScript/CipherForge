# meCloak 🔐

A lightweight encryption tool for secure communication between two parties. One numeric seed generates one unique cipher. Encrypt text messages or any files (images, documents, archives).

## How It Works

1. Enter a numeric seed (e.g., `8374920158`) or generate a random one
2. Share this number with your contact
3. Identical seeds produce identical ciphers
4. For files: the seed is embedded in the `.enc` file, so the recipient doesn't need to type it

## Features

- Deterministic seed-based cipher generation
- Full Unicode support (Cyrillic, Latin, digits, special characters)
- Text encryption (substitution cipher)
- File encryption — images, documents, any file type (XOR gamma)
- Seed verification when decrypting files
- Drag & Drop files onto the window
- File picker dialog (Select File...)
- Save dialog to choose where to save encrypted/decrypted files
- Dark/light theme toggle
- Resizable window with fullscreen support
- Seed strength indicator (red/yellow/green)
- Copy output or seed to clipboard
- Hotkeys: Ctrl+E (Encrypt), Ctrl+D (Decrypt)
- Portable — single .exe, no installation, works offline
