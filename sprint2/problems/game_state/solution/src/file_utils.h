#pragma once



namespace fs = std::filesystem;

// Возвращает true, если каталог p содержится внутри base_path.
bool IsSubPath(fs::path path, fs::path base);