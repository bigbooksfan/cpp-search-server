#include <execution>

#include "process_queries.h"

// ��������� N �������� � ���������� ������ ����� N, i-� ������� �������� � ��������� ������ FindTopDocuments ��� i-�� �������
std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> ret(queries.size());
	
	std::transform(std::execution::par, 
		queries.begin(), queries.end(), 
		ret.begin(), 
		[&](const std::string& query) { return search_server.FindTopDocuments(query); });

	return ret;
}

// ������� ��� ��������� �� ���������� ������ FindTopDocuments ��� ������� �������, ����� ��� ������� � ��� �����
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<Document> ret;
	std::vector<std::vector<Document>> tmp = ProcessQueries(search_server, queries);

	for (const std::vector<Document>& i : tmp) {
		for (const Document& j : i) {
			ret.push_back(j);
		}
	}

	return ret;
}
