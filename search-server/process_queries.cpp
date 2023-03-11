#include <execution>

#include "process_queries.h"

// принимает N запросов и возвращает вектор длины N, i-й элемент которого Ч результат вызова FindTopDocuments дл€ i-го запроса
std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> ret(queries.size());
	
	std::transform(std::execution::par, 
		queries.begin(), queries.end(), 
		ret.begin(), 
		[&](const std::string& query) { return search_server.FindTopDocuments(query); });

	return ret;
}

// сначала все документы из результата вызова FindTopDocuments дл€ первого запроса, затем дл€ второго и так далее
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
