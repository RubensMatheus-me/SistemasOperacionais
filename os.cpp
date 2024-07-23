#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


namespace OS {

// ---------------------------------------
Arch::Terminal *terminalSystem;
Arch::Cpu *cpuSystem;

static std::string bufferedChar;

void boot (Arch::Terminal *terminal, Arch::Cpu *cpu)
{	
	terminalSystem = terminal;
	cpuSystem = cpu;
	terminal->println(Arch::Terminal::Type::Command, "Type commands here");
	terminal->println(Arch::Terminal::Type::App, "Apps output here");
	terminal->println(Arch::Terminal::Type::Kernel, "Kernel output here");
}

// ---------------------------------------

void interrupt(const Arch::InterruptCode interrupt) {
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
            
            if (bufferedChar == "quit") {
            	cpuSystem->Cpu::turn_off();
            }
            bufferedChar.clear(); 
            
            const uint16_t binario = load_from_disk_to_16bit_buffer ("binario");
        }
    }
}


// ---------------------------------------

void syscall () {
    const uint16_t syscall_code = cpuSystem->get_gpr(0); 
    switch (syscall_code) {
        case 0:
            terminalSystem->print(Arch::Terminal::Type::App, "Syscall 0 executado");
            break;
        case 1:
            terminalSystem->print(Arch::Terminal::Type::App, "Syscall 1 executado");
            break;
        default:
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Syscall desconhecido: ");
            break;
	}
	cpuSystem->set_gpr(1, 42);
}

// ---------------------------------------

} // end namespace OS
