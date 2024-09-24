#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


struct process {//bl pc
	uint16_t base;
	uint16_t limit;
	uint16_t pc;
	size_t pid;
	std::array<uint16_t, 8> registers; 
}process;

namespace OS {

// ---------------------------------------
Arch::Terminal *terminalSystem;
Arch::Cpu *cpuSystem;

uint32_t max_size = 65535;
static std::string bufferedChar;

uint16_t get_process_limit(const std::string_view fname) {
    uint32_t file_size_words = Lib::get_file_size_words(fname);
    
    if (file_size_words > max_size) { 
        throw std::runtime_error("O arquivo excedeu o limite maximo da memoria no proceso");
    }
    
    return static_cast<uint16_t>(file_size_words);
}

void load_program(const std::string_view binary_file) {
    process.base = 0;
    process.limit = get_process_limit(binary_file);

    cpuSystem->set_vmem_paddr_init(process.base);
    cpuSystem->set_vmem_paddr_end(process.base + process.limit);
    
    std::vector<uint16_t> loadBinary = Lib::load_from_disk_to_16bit_buffer(binary_file);
}

void execute_instruction() {
	uint16_t instruction = cpuSystem->pmem_read(process.pc + process.base);
	process.pc++;
}

void boot (Arch::Terminal *terminal, Arch::Cpu *cpu)
{
	terminalSystem = terminal;
	cpuSystem = cpu;
	
	 const std::string_view binary_file = "binario.bin";
	
	process.pc = 0;
	process.base = 0;
	process.limit = get_process_limit(binary_file);

	cpuSystem->set_vmem_paddr_init(process.base);
	cpuSystem->set_vmem_paddr_end(process.base + process.limit);
	
	
	terminal->println(Arch::Terminal::Type::Command, "Type commands here");
	terminal->println(Arch::Terminal::Type::App, "Apps output here");
	terminal->println(Arch::Terminal::Type::Kernel, "Kernel output here");
	
	std::vector<uint16_t> loadBinary = Lib::load_from_disk_to_16bit_buffer(binary_file);
	
	for (uint16_t i = 0; i < loadBinary.size(); ++i) {
		if (i + process.base >= process.limit) { 
		    terminalSystem->println(Arch::Terminal::Type::Kernel, "Binário passou a memória limite");
		    break;
		}
		terminalSystem->println(Arch::Terminal::Type::App, i, "- ", loadBinary[i]);
		cpuSystem->pmem_write(i + process.base, loadBinary[i]); 
	}
	
	while (true) {
        execute_instruction();
        
        if (process.pc >= process.limit) {
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Fim do programa.");
            break;
        }
    }
}
 
void kill_process() {
	cpuSystem->set_vmem_paddr_init(0);
	cpuSystem->set_vmem_paddr_end(0);
}

void info_process() {
	terminalSystem->println(Arch::Terminal::Type::Kernel, "Informações do processo:");
    terminalSystem->println(Arch::Terminal::Type::Kernel, "PID: ", process.pid);
    terminalSystem->println(Arch::Terminal::Type::Kernel, "Base: ", process.base);
    terminalSystem->println(Arch::Terminal::Type::Kernel, "Limite: ", process.limit);
    terminalSystem->println(Arch::Terminal::Type::Kernel, "PC: ",  process.pc);
    
    for (size_t i = 0; i < process.registers.size(); ++i) {
        terminalSystem->println(Arch::Terminal::Type::Command, "R", i, ": ", process.registers[i]);
    }
    
}
// ---------------------------------------	

void interrupt(const Arch::InterruptCode interrupt) {
	if(interrupt == Arch::InterruptCode::GPF) {
		terminalSystem->println(Arch::Terminal::Type::Kernel, "Falha geral, matando o processo.");
	}
	
    if (interrupt == Arch::InterruptCode::Keyboard) {
        int typed_char = terminalSystem->read_typed_char();
        char char_typed = static_cast<char>(typed_char);

        if (terminalSystem->is_backspace(typed_char)) {
            if (!bufferedChar.empty()) {
            
                bufferedChar.pop_back();
                 for (size_t i = 0; i < bufferedChar.size() + 1	; ++i) {
                 	terminalSystem->print(Arch::Terminal::Type::Command, '\r'); 
                 	terminalSystem->print(Arch::Terminal::Type::Command, bufferedChar);
                 }
            }
            terminalSystem->println(Arch::Terminal::Type::Kernel, "apagar");
        }

        else if (terminalSystem->is_alpha(typed_char) || terminalSystem->is_num(typed_char)) {
            bufferedChar += char_typed;
            terminalSystem->print(Arch::Terminal::Type::Command, char_typed);
        }

        else if (terminalSystem->is_return(typed_char)) {
			terminalSystem->print(Arch::Terminal::Type::Command, '\n');
      
            terminalSystem->println(Arch::Terminal::Type::Kernel, "enter");
            terminalSystem->print(Arch::Terminal::Type::App, bufferedChar);
            terminalSystem->println(Arch::Terminal::Type::App, "");
            
            if (bufferedChar.rfind("load", 0) == 0) { 
                std::string_view filename = bufferedChar.substr(4) + ".bin";
                
                load_program(filename);
            } else if (bufferedChar == "killprocess") {
                kill_process();
            } else if (bufferedChar == "infoprocess") {
                info_process();
            } else if (bufferedChar == "quit") {
                cpuSystem->Cpu::turn_off();
            }
            
            bufferedChar.clear(); 
        }
       
    }
}


// ---------------------------------------

void syscall() {
    uint16_t r0 = cpuSystem->get_gpr(0);

    switch (r0) {
        case 0:
            cpuSystem->Cpu::turn_off();
            break;
        case 1: {
            uint16_t addr = cpuSystem->get_gpr(1);
             if (addr >= process.base && addr < process.base + process.limit) {
                std::string str;
                char c;
                while ((c = static_cast<char>(cpuSystem->pmem_read(addr))) != '\0') {
                    str += c;
                    addr++;
                }
                terminalSystem->print(Arch::Terminal::Type::App, str);
            } else {
                terminalSystem->println(Arch::Terminal::Type::Kernel, "Error: Address out of bounds.");
            }
            break;
        }
        case 2:
            terminalSystem->println(Arch::Terminal::Type::App, "");
            break;
        case 3: {
            int number = cpuSystem->get_gpr(1);
            terminalSystem->print(Arch::Terminal::Type::App, number);
            break;
        }
        default:
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Unknown syscall");
            break;
    }
}




// ---------------------------------------

} // end namespace OS
