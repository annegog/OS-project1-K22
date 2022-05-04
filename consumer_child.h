#pragma once

#define TEXT_SZ 100

struct shared_use_st {
	char some_text[TEXT_SZ];
	int N_processes;
    int lines_of_file;
	int line;
	int childs;
	int counter_childs;
};

int main();