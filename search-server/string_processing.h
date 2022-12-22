#pragma once

#include <string>
#include <vector>
#include <iostream>

std::string ReadLine();
int ReadLineWithNumber();		// ���, ��� ����� ��������� string �� ����� �� �������. 
//�� ���� ������� ���������� Readline(). ����� �� ����� �����, ���� ��� ����� ������ �����?

std::vector<std::string> SplitIntoWords(const std::string& text);