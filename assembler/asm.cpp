#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>

enum class insn_type {
	imm,
	mov,
	cal,
	jump,
	mem,
	unknown,
};

std::map<std::string, uint8_t> label_table;

void skip_space(std::string_view &src){
	size_t count = 0;
	for(const auto &c : src){
		if(!std::isspace(c))
			break;
		count++;
	}
	src.remove_prefix(count);
}

std::string_view get_token(std::string_view &src){
	size_t count = 0;

	if(src.front()==':' || src.front()==','){
		const auto t = src.substr(0,1);
		src.remove_prefix(1);
		return t;
	}

	for(const auto &c : src){
		if(std::isspace(c) || c==':' || c==',')
			break;
		count++;
	}
	const auto token = src.substr(0, count);
	src.remove_prefix(count);

	return token;
}

void tokenize(std::string_view src, std::vector<std::string_view> &tokens){
	while(!src.empty()){
		skip_space(src);
		if(src.starts_with("//"))	// ignore comment
			break;
		const auto t = get_token(src);
		if(t.empty())
			break;
		tokens.push_back(t);
	}
}

std::pair<insn_type, uint8_t> parse_opcode(std::string_view op){
	if(op == "mov")
		return {insn_type::mov, 0x00};
	else if(op == "add")
		return {insn_type::cal, 0x01};
	else if(op == "sub")
		return {insn_type::cal, 0x02};
	else if(op == "and")
		return {insn_type::cal, 0x03};
	else if(op == "or")
		return {insn_type::cal, 0x04};
	else if(op == "not")
		return {insn_type::cal, 0x05};
	else if(op == "sll")
		return {insn_type::cal, 0x06};
	else if(op == "srl")
		return {insn_type::cal, 0x07};
	else if(op == "sra")
		return {insn_type::cal, 0x08};
	else if(op == "cmp")
		return {insn_type::cal, 0x09};
	else if(op == "je")
		return {insn_type::jump,0x0a};
	else if(op == "jmp")
		return {insn_type::jump,0x0b};
	else if(op == "ldih")
		return {insn_type::imm, 0x0c};
	else if(op == "ldil")
		return {insn_type::imm, 0x0d};
	else if(op == "ld")
		return {insn_type::mem, 0x0e};
	else if(op == "st")
		return {insn_type::mem, 0x0f};

	std::cerr << "error: unknown opcode: \"" << op << "\"" << std::endl;
	return {insn_type::unknown, 0x00};
}

uint8_t get_addr(std::string_view addr_sv){
	std::string addr_s;
	for(const auto &c : addr_sv)
		addr_s.push_back(c);
	const auto it = label_table.find(addr_s);
	if(it == label_table.cend()){
		std::cerr << "error: unknown label \"" << addr_s << "\"" << std::endl;
		return 0x00;
	}
	return it->second;
}

uint8_t reg_index(std::string_view rname){
	if(rname == "r0")		return 0x00;
	else if(rname == "r1")	return 0x01;
	else if(rname == "r2")	return 0x02;
	else if(rname == "r3")	return 0x03;

	std::cerr << "unknown register name: \"" << rname << "\"" << std::endl;
	return 0x00;
}

uint8_t parse_operand(insn_type itype, const std::vector<std::string_view> &tokens){
	switch(itype){
		case insn_type::unknown:
			return 0x00;
		case insn_type::jump:
			return get_addr(tokens[1]);
		case insn_type::cal:
			{
				if(tokens[2] != ",")
					std::cerr << "error" << std::endl;
				const auto op1 = reg_index(tokens[1]);
				const auto op2 = reg_index(tokens[3]);
				return (op1 << 2) | op2;
			}
			break;
		default:
			return 0x00;
	}
}

uint8_t parse_insn(const std::vector<std::string_view> &tokens){
	const auto [itype, opcode] = parse_opcode(tokens[0]);
	const auto oprnd  = parse_operand(itype, tokens);

	std::cout << "opcode = " << std::hex << (uint32_t)opcode
		<< ", operand = " << (uint32_t)oprnd << std::endl;

	return 1;
}

int main(int argc, char **argv){
	std::ifstream src_file;
	size_t line_number = 0;
	uint8_t addr = 0x00;

	src_file.open(argv[1]);
	if(!src_file){
		std::cerr << "cannot open source file." << std::endl;
		return -1;
	}

	while(!src_file.eof()){
		std::string src_line;
		std::getline(src_file, src_line);
		std::string_view src = src_line;
		line_number++;

		std::vector<std::string_view> tokens;
		tokenize(src, tokens);

		//std::cout << "raw: \"" << src << "\"" << std::endl;
		//for(const auto &t : tokens){
		//	std::cout << "\t\"" << t << "\"" << std::endl;
		//}

		if(tokens.size() == 2 && tokens[1] == ":"){
			const auto &label = tokens[0];
			std::cout << "label definition: " << label
				<< "(0x" << std::hex << (uint32_t)addr << ")" << std::endl;
			label_table.insert(std::make_pair(label, addr));
		}else if(tokens.size() != 0)
			addr += parse_insn(tokens);
	}

	return 0;
}
