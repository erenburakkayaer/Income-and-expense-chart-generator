# C++ Accounting AI Assistant

An interactive, AI-powered command-line financial assistant written in C++17. The program allows users to record, query, and visualize monthly incomes and expenses, and integrates with a local LLM (Ollama) via `libcurl` for chat-based accounting analysis.

---

## Features

- **Data Management**: Fast JSON serialization/deserialization for monthly data using the modern `nlohmann/json` library.
- **Visualization**: Automatic generation and rendering of comparison plots (Income, Expense, Net Profit) via `gnuplot`.
- **AI Integration**: Real-time streaming chat integration with a local **Ollama** server running `llama3.2:3b`.
- **Clean Chronology**: Handles entries chronologically sorted by month and year.
- **Localization**: Supports formatting currencies and numbers with local Turkish Lira (`TL`) styling.

---

## Prerequisites & Dependencies

To build and run the project, ensure you have the following installed on your system:

1. **C++ Compiler**: A compiler supporting C++17 or higher (such as `clang` or `g++`).
2. **libcurl**: For communicating with the Ollama AI server.
3. **nlohmann/json**: For JSON file handling (header-only).
4. **gnuplot**: For rendering charts and visual reviews.

### Installation on macOS (via Homebrew)

brew install curl nlohmann-json gnuplot

---

## Build & Run

### 1. Compile the Project

Use the following command to compile the application and link the required `curl` library:

g++ -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib accounting.cpp -o muhasebe -lcurl

### 2. Run the Application

./muhasebe

---

## Usage Guide

Run the program and type commands directly into the prompt:

- **Add Data (`veri ekle`)**: Prompts you for the year, month, total income, and total expenses to save to the database.
- **Plot Charts (`grafik`)**: Visualizes data for a specific month and opens a high-quality `.png` graph using gnuplot.
- **History (`gecmis veriler`)**: Displays previously saved financial logs.
- **AI Chat (`any custom question`)**: If you type anything else, the AI assistant processes your query using your local Ollama server and provides immediate streaming feedback.
- **Quit (`q`)**: Exits the assistant.

---

## Data Structure

The financial data is persisted locally in `hesap.json` using the following schema:

[
    {
        "month": "February 2020",
        "income": 59000,
        "expense": 3000
    },
    {
        "month": "January 2021",
        "income": 20000,
        "expense": 500
    }
]
