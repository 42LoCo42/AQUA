#include <iostream>
#include <fstream>
#include <vector>

#include "executor.h"
#include "compiler.h"
#include "utils.h"
#include "globals.h"

#include <chrono>
using namespace std;

auto main(int argc, char* argv[]) -> int {
	if(argc < 2) {
		cerr << "Usage: " << argv[0] << " <filename> [options]\n";
		cerr << "Where filename can have one of the following extensions:\n";

		cerr << "aquasrc: compile the file to an aquacomp file with the same name. Options:\n";
		cout << "[path]: Search for modules in that path too.\n";
		cout << "[q | v]: quiet or verbose\n";

		cerr << "aquacomp: execute the program contained in the file. Options:\n";
		cout << "[initial memory]: The string of 1 and 0 that should be loaded into the machine. Defaults to 0.\n";
		cout << "[head position]: The position of the machine's read/write head relative to the initial memory.\n";
		exit(EXIT_FAILURE);
	}

	try {
		string filename = argv[1];
		size_t dot_pos = filename.find_last_of('.');
		string basename = filename.substr(0, dot_pos);
		string extension = filename.substr(dot_pos + 1, filename.size() - (dot_pos + 1));

		if(("." + extension) == AQUA_COMPILED_EXT) {
			Module module(basename);
			Executor e(module);
			if(argc >= 3) {
				e.setMem(argv[2]);
				if(argc >= 4) {
					e.setHeadPos(stoul(argv[3]));
				}
			}
			else {
				e.setMem("0");
			}

			/*
			auto start_time = chrono::high_resolution_clock::now();
			for(size_t i=0; i<10000; ++i) {
				e.step();
			}
			auto stop_time = chrono::high_resolution_clock::now();
			auto elapsed = stop_time - start_time;
			cout << "10000 steps took " << chrono::duration_cast<chrono::microseconds>(elapsed).count() << " µs\n";
			*/

			int input = 0;
			bool execute = true;
			size_t steps = 0;

			// main loop
			while(input != 'q') {
				++steps;
				system("clear");
				e.print();
				cout << steps << " steps\n";
				if(execute) {
					execute = e.step();
					input = getchar();
				} else break;
			}
			cout << endl;
		}
		else if(("." + extension) == AQUA_SOURCE_EXT) {
			string mod_path;
			uint verbosity = 1;

			if(argc >= 3) {
				mod_path = argv[2];
				if(argc >= 4) {
					if(string(argv[3]) == "q") verbosity = 0;
					else if(string(argv[3]) == "v") verbosity = 2;
				}
			}

			vector<string> result {};
			Compiler::compile(filename, result, mod_path, verbosity);
			if(verbosity == 2) {
				cout << "\n\n";
				for(auto& s : result) {
					cout << s << '\n';
				}
			}
			Utils::writeFile(basename + AQUA_COMPILED_EXT, result);
			cout << endl;
		}

	} catch (runtime_error& e) {
		cerr << '\n' << FAULT_TEXT << "Fault in " << e.what() << DEFAULT_TEXT << endl << endl;
	}
}
