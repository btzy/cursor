#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

#include <cursordriver/cursordriver.hpp>

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "Not enough arguments.  Expected input file name followed by output file name." << std::endl;
		return 0;
	}
	/*{
		std::cout << "Press enter to continue..." << std::endl;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}*/
	std::vector<unsigned char> inputvec;
	{
		std::basic_filebuf<unsigned char> inputfile;
		if (!inputfile.open(argv[1], std::ios_base::binary | std::ios_base::in)) {
			std::cerr << "Cannot open input file." << std::endl;
			return 0;
		}
		inputvec.resize(64);
		std::size_t eaten_size = 0;
		while (true) {
			std::size_t next_amount = inputvec.size() - eaten_size;
			std::size_t read_amount = inputfile.sgetn(inputvec.data() + eaten_size, next_amount);
			eaten_size += read_amount;
			if (read_amount < next_amount)break;
			inputvec.resize(inputvec.size() * 2);
		}
		inputvec.resize(eaten_size);
		inputvec.shrink_to_fit();
	}
	unsigned char* tmp_buf = inputvec.data();
	std::size_t tmp_size = inputvec.size();
	cursor::Options tmp_options;
	tmp_options.hotspot_width = tmp_options.hotspot_height = 0;
	const char* tmp_error_out = nullptr;
	cursor::owning_span<std::byte> result = cursor::MakeCursor(1, &tmp_buf, &tmp_size, &tmp_options, tmp_error_out);
	if (tmp_error_out) {
		std::cerr << tmp_error_out << std::endl;
		return 0;
	}
	{
		std::basic_filebuf<std::byte> outputfile;
		if (!outputfile.open(argv[2], std::ios_base::binary | std::ios_base::out)) {
			std::cerr << "Cannot create file for output." << std::endl;
			return 0;
		}
		if (outputfile.sputn(result.get(), result.size()) != result.size()) {
			std::cerr << "Failure while writing to file." << std::endl;
			return 0;
		}
	}
	std::cerr << "Success!" << std::endl;
	return 0;
}