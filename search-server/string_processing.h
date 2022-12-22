#pragma once

#include <string>
#include <vector>
#include <iostream>

std::string ReadLine();
int ReadLineWithNumber();		// нет, это часть обработки string со входа из консоли. 
//Из этой функции вызывается Readline(). Разве не будет лучше, если они будут лежать рядом?

std::vector<std::string> SplitIntoWords(const std::string& text);