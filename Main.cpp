#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stack>
#include <map>
#include <stdexcept>
#include <algorithm>

// --- Configuration ---
// Set to true to see detailed program execution (PC and instructions)
const bool DEBUG_MODE = true;

// --- Virtual Machine State ---
class VM {
public:
std::vector<int> memory;
std::stack<int> system_stack;
int program_counter;
bool is_running;

// CPU Flags for conditional logic
struct Flags {
bool zero; // Set if the result of a CMP is zero (equal)
} flags;

VM() : memory(256, 0), program_counter(0), is_running(false) {
flags.zero = false;
}

void reset() {
memory.assign(256, 0);
while(!system_stack.empty()) system_stack.pop();
program_counter = 0;
is_running = false;
flags.zero = false;
}
};

// --- Helper Functions & Classes ---

// Serial output function for cleaner logging
void serial_log(const std::string& message, bool is_debug_msg = false) {
if (is_debug_msg && !DEBUG_MODE) {
return;
}
std::cout << message << std::endl;
}

// A simple struct to represent a parsed instruction
struct Instruction {
std::string opcode;
std::vector<std::string> args;
};

// --- Assembler ---
// Translates assembly code with labels into executable bytecode (for our VM)
class Assembler {
private:
std::map<std::string, int> label_map;

// Helper to trim whitespace from a string
std::string trim(const std::string& str) {
size_t first = str.find_first_not_of(" \t\n\r");
if (std::string::npos == first) return str;
size_t last = str.find_last_not_of(" \t\n\r");
return str.substr(first, (last - first + 1));
}

public:
std::vector<Instruction> assemble(const std::vector<std::string>& source_code) {
label_map.clear();
std::vector<std::string> clean_code;
std::vector<Instruction> instructions;

// --- First Pass: Find all labels ---
int line_number = 0;
for (const auto& line : source_code) {
std::string trimmed_line = trim(line);
// Skip comments and empty lines
if (trimmed_line.empty() || trimmed_line[0] == '#') {
continue;
}

// Check for a label (e.g., "my_label:")
size_t label_pos = trimmed_line.find(':');
if (label_pos != std::string::npos) {
std::string label = trim(trimmed_line.substr(0, label_pos));
if (label_map.count(label)) {
throw std::runtime_error("Assembler error: Duplicate label '" + label + "'");
}
label_map[label] = line_number;
// Remove the label part for the next step
trimmed_line = trim(trimmed_line.substr(label_pos + 1));
}
if (!trimmed_line.empty()) {
clean_code.push_back(trimmed_line);
line_number++;
}
}

// --- Second Pass: Parse instructions and resolve labels ---
for (const auto& line : clean_code) {
std::stringstream ss(line);
Instruction inst;
ss >> inst.opcode;
std::transform(inst.opcode.begin(), inst.opcode.end(), inst.opcode.begin(), ::toupper);

std::string arg;
while (ss >> arg) {
if (arg.back() == ',') arg.pop_back();
// If it's a jump/call instruction, try to resolve the label
if (inst.opcode == "JUMP" || inst.opcode == "CALL" || inst.opcode == "JZE" || inst.opcode == "JNE") {
if (label_map.count(arg)) {
arg = std::to_string(label_map[arg]);
}
}
inst.args.push_back(arg);
}
instructions.push_back(inst);
}

serial_log("Assembled " + std::to_string(instructions.size()) + " instructions.", true);
return instructions;
}
};


// --- Instruction Executor ---
void execute_instruction(VM& vm, const Instruction& inst) {
const std::string& op = inst.opcode;
const std::vector<std::string>& args = inst.args;

try {
if (op == "PRINT") {
int addr = std::stoi(args.at(0));
serial_log(std::to_string(vm.memory.at(addr)));
} else if (op == "LOAD") { // LOAD <dest_addr> <src_addr>
int dest = std::stoi(args.at(0));
int src = std::stoi(args.at(1));
vm.memory.at(dest) = vm.memory.at(src);
} else if (op == "LOADI") { // LOADI <dest_addr> <value>
int addr = std::stoi(args.at(0));
int val = std::stoi(args.at(1));
vm.memory.at(addr) = val;
} else if (op == "ADD") {
int addr1 = std::stoi(args.at(0));
int addr2 = std::stoi(args.at(1));
vm.memory.at(addr1) += vm.memory.at(addr2);
} else if (op == "SUB") {
int addr1 = std::stoi(args.at(0));
int addr2 = std::stoi(args.at(1));
vm.memory.at(addr1) -= vm.memory.at(addr2);
} else if (op == "MUL") {
int addr1 = std::stoi(args.at(0));
int addr2 = std::stoi(args.at(1));
vm.memory.at(addr1) *= vm.memory.at(addr2);
} else if (op == "CMP") { // Compare values at two addresses
int val1 = vm.memory.at(std::stoi(args.at(0)));
int val2 = vm.memory.at(std::stoi(args.at(1)));
vm.flags.zero = (val1 == val2);
} else if (op == "JUMP") {
vm.program_counter = std::stoi(args.at(0)) - 1; // -1 to offset PC increment
} else if (op == "JZE") { // Jump if Zero/Equal
if (vm.flags.zero) {
vm.program_counter = std::stoi(args.at(0)) - 1;
}
} else if (op == "JNE") { // Jump if Not Equal
if (!vm.flags.zero) {
vm.program_counter = std::stoi(args.at(0)) - 1;
}
} else if (op == "PUSH") { // Push value from memory
vm.system_stack.push(vm.memory.at(std::stoi(args.at(0))));
} else if (op == "PUSHI") { // Push immediate value
vm.system_stack.push(std::stoi(args.at(0)));
} else if (op == "POP") { // Pop value to memory
if (vm.system_stack.empty()) throw std::runtime_error("Stack underflow");
vm.memory.at(std::stoi(args.at(0))) = vm.system_stack.top();
vm.system_stack.pop();
} else if (op == "CALL") {
vm.system_stack.push(vm.program_counter + 1); // Push return address
vm.program_counter = std::stoi(args.at(0)) - 1;
} else if (op == "RET") {
if (vm.system_stack.empty()) throw std::runtime_error("Stack underflow on RET");
vm.program_counter = vm.system_stack.top() - 1;
vm.system_stack.pop();
} else if (op == "HALT") {
vm.is_running = false;
} else {
throw std::runtime_error("Unknown instruction '" + op + "'");
}
} catch (const std::out_of_range& oor) {
throw std::runtime_error("Execution error for " + op + ": Invalid memory address or argument count.");
} catch (const std::invalid_argument& ia) {
throw std::runtime_error("Execution error for " + op + ": Invalid number format in arguments.");
}
}

// --- Main Program Logic ---

// Load program source code from a file
std::vector<std::string> load_source_from_file(const std::string& filename) {
std::ifstream file(filename);
if (!file.is_open()) {
throw std::runtime_error("Error: Could not open file '" + filename + "'");
}
std::vector<std::string> source;
std::string line;
while (std::getline(file, line)) {
source.push_back(line);
}
file.close();
return source;
}

void run_program(VM& vm, const std::vector<Instruction>& program) {
serial_log("--- Executing Program ---", true);
vm.is_running = true;
vm.program_counter = 0;

while (vm.is_running && vm.program_counter < program.size()) {
const auto& inst = program[vm.program_counter];
std::string debug_line = "PC:" + std::to_string(vm.program_counter) + " > " + inst.opcode;
for(const auto& arg : inst.args) debug_line += " " + arg;
serial_log(debug_line, true);

try {
execute_instruction(vm, inst);
} catch (const std::runtime_error& e) {
serial_log("FATAL ERROR at line " + std::to_string(vm.program_counter) + ": " + e.what());
vm.is_running = false;
}
vm.program_counter++;
}

if (vm.is_running) {
serial_log("--- Program Completed ---", true);
} else {
serial_log("--- Program Halted ---", true);
}
}

void dump_memory(const VM& vm, int start = 0, int count = 16) {
serial_log("\n--- Memory Dump (Non-zero) ---", true);
for (int i = start; i < start + count && i < vm.memory.size(); i++) {
if (vm.memory[i] != 0) {
serial_log("MEM[" + std::to_string(i) + "] = " + std::to_string(vm.memory[i]), true);
}
}
serial_log("--- End Dump ---", true);
}


int main(int argc, char* argv[]) {
if (argc < 2) {
std::cerr << "Usage: " << argv[0] << " <program.asm>" << std::endl;
return 1;
}

std::cout << "--- Welcome to ContainMe! v3.0 ---" << std::endl;
VM virtual_machine;
Assembler assembler;

try {
std::vector<std::string> source = load_source_from_file(argv[1]);
std::vector<Instruction> program = assembler.assemble(source);
run_program(virtual_machine, program);
} catch (const std::runtime_error& e) {
std::cerr << "Error: " << e.what() << std::endl;
return 1;
}

if (DEBUG_MODE) {
dump_memory(virtual_machine);
}
return 0;
}
