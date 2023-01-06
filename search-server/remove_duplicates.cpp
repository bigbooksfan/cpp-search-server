#include "remove_duplicates.h"

#include <vector>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server)
{

	std::vector<int> ids_to_remove = search_server.CheckDuplicatesInside();

	for (int i : ids_to_remove) {
		search_server.RemoveDocument(i);
		std::cout << "Found duplicate document id " << i << std::endl;
	}

}
