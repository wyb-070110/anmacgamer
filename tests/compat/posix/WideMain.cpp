// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Test programs off Windows that are written with wmain: main turns the arguments into wide strings.
#include <string>
#include <string_view>
#include <vector>

int wmain(int argc, wchar_t** argv);

int main(int argc, char** argv)
{
    std::vector<std::wstring> words;
    for (int index = 0; index < argc; ++index) {
        const std::string_view word(argv[index]);
        words.emplace_back(word.begin(), word.end());
    }
    std::vector<wchar_t*> pointers;
    for (std::wstring& word : words) {
        pointers.push_back(word.data());
    }
    pointers.push_back(nullptr);
    return wmain(argc, pointers.data());
}
